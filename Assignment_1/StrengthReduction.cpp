#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

bool strengthReduction(Instruction &I) {
	if (auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
		auto Opcode = BinOp->getOpcode();
		if (Opcode == Instruction::Mul || Opcode == Instruction::UDiv || Opcode == Instruction::SDiv) {
			bool isMul = (Opcode == Instruction::Mul);
			for (int i = 0; i < 2; i++) {
				if (auto *ConstOp = dyn_cast<ConstantInt>(BinOp->getOperand(i))) {
					Value *altroOperando = BinOp->getOperand(1 - i);
					int base = ConstOp->getValue().exactLogBase2();
					if (isMul) {
						int basepiu = (ConstOp->getValue() + 1).exactLogBase2();
						int basemeno = (ConstOp->getValue() - 1).exactLogBase2();
	
						if (base != -1 || basepiu != -1 || basemeno != -1) {
							int shift = base != -1 ? base : (basepiu != -1 ? basepiu : basemeno);
							ConstantInt *ShiftAmount = ConstantInt::get(I.getContext(), APInt(32, shift));
	
							Instruction *ShiftInst = BinaryOperator::Create(
									Instruction::Shl, altroOperando, ShiftAmount);
	
							ShiftInst->insertBefore(&I);
	
							if (basepiu != -1) {								
									ConstantInt *AddAmount = ConstantInt::get(I.getContext(), APInt(32, 1));
	
									Instruction *AddInst = BinaryOperator::Create(
											Instruction::Add, ShiftInst, AddAmount);
	
									AddInst->insertBefore(&I);
									I.replaceAllUsesWith(AddInst);
									
							} else if (basemeno != -1) {								
									ConstantInt *SubAmount = ConstantInt::get(I.getContext(), APInt(32, 1));
	
									Instruction *SubInst = BinaryOperator::Create(
											Instruction::Sub, ShiftInst, SubAmount);
	
									SubInst->insertBefore(&I);
									I.replaceAllUsesWith(SubInst);
	
							} else {
									I.replaceAllUsesWith(ShiftInst);
							}
							I.eraseFromParent();
							return true;
						}
					} else {
            if (base != -1) {
              ConstantInt *ShiftAmount = ConstantInt::get(I.getContext(), APInt(32, base));
              Instruction *ShiftInst = BinaryOperator::Create((Opcode == Instruction::SDiv) ? Instruction::AShr : Instruction::LShr, altroOperando, ShiftAmount);
              ShiftInst->insertBefore(&I);
              I.replaceAllUsesWith(ShiftInst);
              I.eraseFromParent();
              return true;
            }
          }		
				}
			}
		}
	}
	return false;
}

bool runOnFunction(Function &F) {
	bool Transformed = false;
	for (auto &BB : F) {
		for (auto it = BB.begin(), end = BB.end(); it != end;) {
			Instruction &I = *it++;
			if (strengthReduction(I)) {
				Transformed = true;
			}
		}
	}
	return Transformed;
}

namespace {

	struct TestPass : PassInfoMixin<TestPass> {		
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

		if (runOnFunction(F)) errs() << "Strength Reduction Done\n";
		else                  errs() << "No Strength Reduction\n";
		
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
                  if (Name == "strength-reduction") {
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
