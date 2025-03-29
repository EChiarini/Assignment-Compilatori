#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

  bool AlgebraicIdentity(Instruction &I) {
    if (I.getOpcode() != Instruction::Mul && I.getOpcode() != Instruction::Add) {
      return false;
    }
    for (int i = 0; i < 2; i++) {
      if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(i))) {
        if ((I.getOpcode() == Instruction::Mul && CI->getValue() == 1)
          || (I.getOpcode() == Instruction::Add && CI->getValue() == 0)) {
          I.replaceAllUsesWith(I.getOperand(1 - i));
          I.eraseFromParent();
          return true;
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
				if (AlgebraicIdentity(I)) {
					Transformed = true;
				}
			}
		}
		return Transformed;
	}

namespace {
  struct TestPass: PassInfoMixin<TestPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    if (runOnFunction(F)) errs() << "Algebraic Identity Semplification Done\n";
    else                  errs() << "No Algebraic Identity Semplification\n";

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
                  if (Name == "algebraic-identity") {
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
