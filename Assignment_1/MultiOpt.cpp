#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
LLVMContext Context;

Value *ricercaRicorsivaMatrice(std::vector<std::vector<Value*>> m, Value *reg, int iterationIndex, int c, 
								std::vector<Instruction*> &instructionsInChain) {
	//al contrario
  for (int i = iterationIndex - 1; i >= 0; i--) {
		BinaryOperator *previous_inst = dyn_cast<BinaryOperator>(m[i][0]);
		BinaryOperator *current_inst = dyn_cast<BinaryOperator>(reg);

		if (!previous_inst || !current_inst || !previous_inst->isIdenticalTo(current_inst)) {
			continue;
		}

		//Se non ho valore costante
		ConstantInt* CI = dyn_cast<ConstantInt>(m[i][2]);
		if (!CI) continue;

		int intValue = CI->getZExtValue();
		if (previous_inst->getOpcode() == Instruction::Sub) {
			intValue = -intValue;
		}
		int quantoManca  = c + intValue;

		//la metto nella mia lista
		instructionsInChain.push_back(previous_inst);

		//se la somma di tutte le operazioni è 0 -> tempo di eliminare
		if (quantoManca == 0) {
			return m[i][1];
		}

		// se no vado avanti
		Value *result = ricercaRicorsivaMatrice(m, m[i][1], i, quantoManca, instructionsInChain);
		if (result) return result;

		//istruzione inutile
		instructionsInChain.pop_back();
	}
	return nullptr;
}

bool runOnBasicBlock(BasicBlock &B) {
	std::vector<std::vector<Value*>> instructions;
	std::vector<Instruction*> toErase;
	std::vector<std::pair<Instruction*, Value*>> replacements;

	//per ogni istruzione, la metto nella mia matrica se è binaria e ha un operando costante
	for (auto &I : B) {
		BinaryOperator *binOp = dyn_cast<BinaryOperator>(&I);
		if (binOp && (binOp->getOpcode() == Instruction::Add || binOp->getOpcode() == Instruction::Sub)) {
			if (isa<Constant>(binOp->getOperand(1))) {
				instructions.push_back({binOp, binOp->getOperand(0), binOp->getOperand(1)});
			} else if (isa<Constant>(binOp->getOperand(0))) {
				instructions.push_back({binOp, binOp->getOperand(1), binOp->getOperand(0)});
			}
		}
	}

	//per ogni istruzione "papabile"
	for (int i = instructions.size() - 1; i >= 0; i--) {
		auto *binOp = cast<BinaryOperator>(instructions[i][0]);
		//se è una sottrazione voglio il valore col meno
		auto *constVal = dyn_cast<ConstantInt>(instructions[i][2]);
		int intValue = binOp->getOpcode() == Instruction::Sub ? 
						-constVal->getSExtValue() : constVal->getSExtValue();

		std::vector<Instruction*> instructionsInChain;
		if (Value *result = ricercaRicorsivaMatrice(instructions, instructions[i][1], 
														i, intValue, instructionsInChain)) {
			replacements.emplace_back(binOp, result);
			toErase.push_back(binOp);
			toErase.insert(toErase.end(), instructionsInChain.begin(), instructionsInChain.end());
		}
	}

	for (const auto &[inst, replacement] : replacements) {
		inst->replaceAllUsesWith(replacement);
	}

	for (auto *inst : toErase) {
		inst->eraseFromParent();
	}

	return !toErase.empty();
}

namespace {

	struct TestPass : PassInfoMixin<TestPass> {		
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
			bool Transformed = false;

			for (auto BB = F.begin(); BB != F.end(); ++BB) {
				if (runOnBasicBlock(*BB)) {
					Transformed = true;
				}
			}
			
			return PreservedAnalyses::all();
		}

		static bool isRequired() { return true; }
	};
}

llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "TestPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "multi-opt") {
                    FPM.addPass(TestPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getTestPassPluginInfo();
}
