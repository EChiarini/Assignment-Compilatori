#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include <vector>
#include <set>
#include <map>

using namespace llvm;

void prendiVariLoop(Loop *L, std::vector<Loop*> &WorkList) {
  for (Loop *SubLoop : *L) {
      prendiVariLoop(SubLoop, WorkList);
  }

  WorkList.push_back(L);
}

bool seLoopInvariante(Instruction *Inst, Loop *L, const std::set<Instruction*>& LoopInvariants) {
  for (Use &U : Inst->operands()) {
      Value *Operand = U.get();
            if (isa<Constant>(Operand) || isa<Argument>(Operand)) {
          continue;
      }
      
      Instruction *OperandInst = dyn_cast<Instruction>(Operand);
      if (L->contains(OperandInst)) {
          if (LoopInvariants.find(OperandInst) == LoopInvariants.end()) {
              return false;
          }
      }
  }
  return true;
}

bool noUsiFuoriLoop(Instruction *I, Loop *L) {
  for (User *U : I->users()) {
    Instruction *Use = dyn_cast<Instruction>(U);
    if (Use && !L->contains(Use)) {
      return false;
    }
  }
  return true;
}

bool dominaTutteUscite(Instruction *I, Loop *L, DominatorTree &DT) {
  SmallVector<BasicBlock*, 4> UniqueExitBlocks;
  L->getUniqueExitBlocks(UniqueExitBlocks);
    for (BasicBlock *Succ : UniqueExitBlocks) {
    if (!DT.dominates(I, Succ)) {
      return false;
    }
  }
  return true;
}

bool dominaUsiDentroLoop(Instruction *I, Loop *L, DominatorTree &DT) {
  for (User *U : I->users()) {
    Instruction *Use = dyn_cast<Instruction>(U);
    if (Use && L->contains(Use)) {
      if (!DT.dominates(I, Use)) {
        return false;
      }
    }
  }
  return true;
}

namespace { struct FakeLICM : PassInfoMixin<FakeLICM> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
      bool mod = false;
      LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
      DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
      std::vector<Loop*> WorkList;
      // LISTA LOOP
      for (Loop *TopLevelLoop : LI) {
          prendiVariLoop(TopLevelLoop, WorkList);
      }

      for (auto *L: WorkList) {
        std::set<Instruction*> LoopInvariants;
        errs() << "Loop a profondita' " << L->getLoopDepth() << "\n";
        // LOOP INVARIANT
        for (auto *BB : L->getBlocks()) {
          //for (Instruction &I : *BB) {
          for (auto it = BB->begin(); it != BB->end(); ) {
            Instruction &I = *it++;
          
            if (I.isBinaryOp()) {
              if (seLoopInvariante(&I, L, LoopInvariants)) { // LOOP INVARIANT          
                if ((dominaTutteUscite(&I,L,DT) || noUsiFuoriLoop(&I,L)) && dominaUsiDentroLoop(&I,L,DT)) { // CODE MOTION
                  errs() << "Istruzione" << I << " è Loop Invariant Code Motion,";
                  mod = true;
                  BasicBlock *Preheader = L->getLoopPreheader();
                  I.moveBefore(&*Preheader->getTerminator());
                  errs() << "e diventa" << I << "\n" ;
                } else  {
                  errs() << "Istruzione " << I << " è Loop Invariant, ma non Code Motion\n";
                }
                
              }
            }
          }
        }
        
        // CODE MOTION MODO ALTERNATIVO
        /*
        for (auto it = LoopInvariants.begin(); it != LoopInvariants.end();) {
          Instruction* I = *it;
          BasicBlock *BB = I->getParent();
          if ((dominaTutteUscite(I,L,DT) || noUsiFuoriLoop(I,L)) && dominaUsiDentroLoop(I,L,DT)) {
            errs() << "Istruzione" << *I << " è Loop Invariant Code Motion,";
            mod = true;
            BasicBlock *Preheader = L->getLoopPreheader();
            I->moveBefore(&*Preheader->getTerminator());
            it = LoopInvariants.erase(it);
            errs() << "e diventa" << *I << "\n" ;
          } else {
            ++it;
          }
        }
        */
      }

      return mod ? PreservedAnalyses::none() : PreservedAnalyses::all();
      
    }

    static bool isRequired() { return true; } 
}; }

PassPluginLibraryInfo getFakeLICMPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "FakeLICM", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "fake-licm") {
                    FPM.addPass(FakeLICM());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFakeLICMPluginInfo();
}