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
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include <vector>
#include <set>
#include <map>
#include <queue>


using namespace llvm;

BasicBlock *prendiHeader(Loop *Loop, bool guardia) {
  if (guardia) {
    BasicBlock *PreHeader = Loop->getLoopPreheader();
    BasicBlock *Guardia = PreHeader->getSinglePredecessor();
    return Guardia;
  } 
  return Loop->getHeader();
}

bool stessaCondizione(BranchInst *B1, BranchInst *B2) {
  Value *Cond1 = B1->getCondition();
  Value *Cond2 = B2->getCondition();

  if (Cond1 == Cond2)
    return true;

  if (ICmpInst *Cmp1 = dyn_cast<ICmpInst>(Cond1)) {
    if (ICmpInst *Cmp2 = dyn_cast<ICmpInst>(Cond2)) {
      return Cmp1->getPredicate() == Cmp2->getPredicate() &&
             Cmp1->getOperand(0) == Cmp2->getOperand(0) &&
             Cmp1->getOperand(1) == Cmp2->getOperand(1);
    }
  }
  return false;
}


bool conGuardia(Loop *PrimoLoop, Loop *SecondoLoop) {
  BasicBlock *PreHeaderPrimo = PrimoLoop->getLoopPreheader();
  BasicBlock *PreHeaderSecondo = SecondoLoop->getLoopPreheader();
  
  if (!PreHeaderPrimo || !PreHeaderSecondo) { return false; }

  BasicBlock *GuardiaPrimo = PreHeaderPrimo->getSinglePredecessor();
  BasicBlock *GuardiaSecondo = PreHeaderSecondo->getSinglePredecessor();

  if (!GuardiaPrimo || !GuardiaSecondo) { return false; }

  BranchInst *GuardiaBranch1 = dyn_cast<BranchInst>(GuardiaPrimo->getTerminator());
  BranchInst *GuardiaBranch2 = dyn_cast<BranchInst>(GuardiaSecondo->getTerminator());

  if (!GuardiaBranch1 || !GuardiaBranch2) { return false; }

  if (GuardiaBranch1->isConditional()) {
    BasicBlock *Succ1 = GuardiaBranch1->getSuccessor(0);
    BasicBlock *Succ2 = GuardiaBranch1->getSuccessor(1);

    if ((Succ1 == GuardiaSecondo && Succ2 == PreHeaderPrimo) ||
        (Succ2 == GuardiaSecondo && Succ1 == PreHeaderPrimo)) {

        bool trovata = false;
        for (Instruction &I : *GuardiaSecondo) {
            if (I.isBinaryOp() && !isa<ICmpInst>(I) && !isa<BranchInst>(I)) { //NON CONTA STORE E LOAD PERò 
              trovata = true; 
              break;
            }
        }

        if (trovata) { return false; }
      
      if (stessaCondizione(GuardiaBranch1, GuardiaBranch2)) { return true; }
    }
  }

  return false;
}


bool senzaGuardia(Loop *PrimoLoop, Loop *SecondoLoop) {
  SmallVector<BasicBlock*, 4> PrimoLoopUscite;
  PrimoLoop->getExitBlocks(PrimoLoopUscite);
  bool trovata = false;

  for (BasicBlock* ExitBlock : PrimoLoopUscite) {
    if (ExitBlock != SecondoLoop->getLoopPreheader()) {
      trovata = true;
      break;
    }
  }
  if (trovata) { return false; }

  // VERSIONE SEMPLIFICATA
  // if (PrimoLoop->getExitBlock() != SecondoLoop->getLoopPreheader()) { return false; }

  trovata = false;
  BasicBlock *Preheader = PrimoLoop->getExitBlock();
  for (Instruction &I : *Preheader) {
      if (I.isBinaryOp() && !isa<ICmpInst>(I) && !isa<BranchInst>(I)) { //NON CONTA STORE E LOAD PERò 
        trovata = true; 
        break;
      }
  }

  if (trovata) { return false; }


  return true;
}

bool equivalenzaControlFlow(Loop *PrimoLoop, Loop *SecondoLoop, DominatorTree &DT, PostDominatorTree &PDT, bool guardia) {
  BasicBlock *HeaderPrimo = prendiHeader(PrimoLoop, guardia);
  BasicBlock *HeaderSecondo = prendiHeader(SecondoLoop, guardia);

  if (!DT.dominates(HeaderPrimo, HeaderSecondo)) { return false; }
  if (!PDT.dominates(HeaderSecondo, HeaderPrimo)) { return false; }

  return true;
}

bool numeroIterazioniUguali(Loop *PrimoLoop, Loop *SecondoLoop, ScalarEvolution &SE) {
  const SCEV *iterazioniPrimo = SE.getBackedgeTakenCount(PrimoLoop);
  const SCEV *iterazioniSecondo = SE.getBackedgeTakenCount(SecondoLoop);

  //caso usi parametri di funzione
  if (isa<SCEVCouldNotCompute>(iterazioniPrimo) || isa<SCEVCouldNotCompute>(iterazioniSecondo)) { return false; }

  if (const SCEVConstant *numeroPrimo = dyn_cast<SCEVConstant>(iterazioniPrimo)) {
    if (const SCEVConstant *numeroSecondo = dyn_cast<SCEVConstant>(iterazioniSecondo)) {
      APInt contoPrimo = numeroPrimo->getAPInt();
      APInt contoSecondo = numeroSecondo->getAPInt();
      if (contoPrimo == contoSecondo) { return true; }
    }
  }
  return false;
}

bool dipendenzeDistanzaNegativa(Loop *PrimoLoop, Loop *SecondoLoop, DependenceInfo &DI, ScalarEvolution &SE) {
    SmallPtrSet<Instruction*, 32> PrimeIstruzioni;
    SmallPtrSet<Instruction*, 32> SecondeIstruzioni;

    for (BasicBlock *BB1 : PrimoLoop->blocks()) {
      for (Instruction &I1 : *BB1) {
        Instruction *InstUno = &I1;
        if (InstUno->getOpcode() == Instruction::Store || InstUno->getOpcode() == Instruction::Load) {
          PrimeIstruzioni.insert(InstUno);
        }
      }
    }

    for (BasicBlock *BB2 : SecondoLoop->blocks()) {
      for (Instruction &I2 : *BB2) {
        Instruction *InstDue = &I2;
        if (InstDue->getOpcode() == Instruction::Load || InstDue->getOpcode() == Instruction::Store) {
          SecondeIstruzioni.insert(InstDue);
        }
      }
    }

    for (Instruction *PrimaSingolaIstruzione : PrimeIstruzioni) {
      for (Instruction *SecondaDSingolaIstruzione : SecondeIstruzioni) {
        if (!DI.depends(PrimaSingolaIstruzione, SecondaDSingolaIstruzione, true)) { continue; }

        const SCEV *doveScrivo = SE.getSCEV(PrimaSingolaIstruzione->getOperand(1));
        const SCEV *doveLeggo  = SE.getSCEV(SecondaDSingolaIstruzione->getOperand(0));
        const SCEV *doveInizioScrivo;
        const SCEV *doveInizioLeggo;
        
        if (const SCEVAddRecExpr *doveEffettivamenteInizioScrivere = dyn_cast<SCEVAddRecExpr>(doveScrivo)) {
          doveInizioScrivo = doveEffettivamenteInizioScrivere->getStart();
        } else { continue; }
        if (const SCEVAddRecExpr *doveEffettivamenteInizioLeggere = dyn_cast<SCEVAddRecExpr>(doveLeggo)) {
          doveInizioLeggo = doveEffettivamenteInizioLeggere->getStart();
        } else { continue; }
            
        const SCEV *distanzaInizi = SE.getMinusSCEV(doveInizioScrivo, doveInizioLeggo);
        if (const SCEVConstant *Constdiff = dyn_cast<SCEVConstant>(distanzaInizi)) {
          if (Constdiff->getAPInt().getSExtValue() < 0) { return true; }
        }
      }
    }
    return false;
}

bool unisciLoop(Loop *PrimoLoop, Loop *SecondoLoop, bool guardia) {
  PHINode *VariabilePrimo = PrimoLoop->getCanonicalInductionVariable();
  PHINode *VariabileSecondo = SecondoLoop->getCanonicalInductionVariable();
  VariabileSecondo->replaceAllUsesWith(VariabilePrimo);

  BasicBlock *HeaderPrimo = PrimoLoop->getHeader();
  BasicBlock *HeaderSecondo = SecondoLoop->getHeader();
  BasicBlock *PreHeaderSecondo = SecondoLoop->getLoopPreheader();
  BasicBlock *ExitSecondo = SecondoLoop->getExitBlock();

  Instruction *UltimaIstruzionePrimo = HeaderPrimo->getTerminator();
  BranchInst *BranchPrimo = dyn_cast<BranchInst>(UltimaIstruzionePrimo);

  // NEL CASO ABBIA PIU' PHI NODES
  IRBuilder<> builder(HeaderPrimo->getFirstNonPHI());
  std::vector<PHINode*> phiCestino;
  for (PHINode &PN : HeaderSecondo->phis()) {
    PHINode* oldPHI = &PN;
    if (oldPHI == VariabileSecondo) {
        continue;
    }

    PHINode *newPHI = builder.CreatePHI(oldPHI->getType(), 2, oldPHI->getName() + ".NUOVO");
    Value *initialValue = oldPHI->getIncomingValueForBlock(PreHeaderSecondo);
    newPHI->addIncoming(initialValue, PrimoLoop->getLoopPreheader());

    BasicBlock *latchSecondo = SecondoLoop->getLoopLatch();
    Value *latchValue = oldPHI->getIncomingValueForBlock(latchSecondo);

    newPHI->addIncoming(latchValue, PrimoLoop->getLoopLatch());
    oldPHI->replaceAllUsesWith(newPHI);
    phiCestino.push_back(oldPHI);
  }
  for (PHINode *phi : phiCestino) {
    phi->eraseFromParent();
  }

  if (guardia) {
    // G1 -> E2
    BasicBlock *GuardiaPrimo = prendiHeader(PrimoLoop, guardia);
    BasicBlock *GuardiaSecondo = prendiHeader(SecondoLoop, guardia);

    Instruction *UltimaIstruzioneGuardiaPrimo = GuardiaPrimo->getTerminator();
    BranchInst *BranchGuardiaPrimo = dyn_cast<BranchInst>(UltimaIstruzioneGuardiaPrimo);

    if (BranchGuardiaPrimo->getOperand(0) == GuardiaSecondo) {
      BranchGuardiaPrimo->setOperand(0, ExitSecondo);
    } else if (BranchGuardiaPrimo->getOperand(1) == GuardiaSecondo) {
      BranchGuardiaPrimo->setOperand(1, ExitSecondo);
    } else {
      errs() << "errore in G1 -> E2\n";
      return false;
    }
  } else {
    //H1 -> E2
    if (BranchPrimo->getOperand(0) == PreHeaderSecondo) {
      BranchPrimo->setOperand(0, ExitSecondo);
    } else if (BranchPrimo->getOperand(1) == PreHeaderSecondo) {
      BranchPrimo->setOperand(1, ExitSecondo);
    } else {
      errs() << "errore in H1 -> E2\n";
      return false;
    }
  }

  //B1 -> B2
  BasicBlock *ExitPrimo = PrimoLoop->getExitBlock();
  BasicBlock *BodyPrimo;
  BasicBlock *BodySecondo;
  for (auto BB1 : successors(HeaderPrimo)) {
    if (BB1 != ExitPrimo) {
      BodyPrimo = BB1;
      break;
    }
  }
    for (auto BB2 : successors(HeaderSecondo)) {
    if (BB2 != ExitSecondo) {
      BodySecondo = BB2;
      break;
    }
  }

  Instruction *UltimoBodyPrimo = BodyPrimo->getTerminator();
  BranchInst *BranchBodyPrimo = dyn_cast<BranchInst>(UltimoBodyPrimo);
  BranchBodyPrimo->setSuccessor(0, BodySecondo);

  //B2 -> L1
  Instruction *UltimoBodySecondo = BodySecondo->getTerminator();
  BranchInst *BranchBodySecondo = dyn_cast<BranchInst>(UltimoBodySecondo);
  BranchBodySecondo->setSuccessor(0, PrimoLoop->getLoopLatch());

  //H2 -> L2
  Instruction *UltimaIstruzioneSecondo = HeaderSecondo->getTerminator();
  BranchInst *BranchSecondo = dyn_cast<BranchInst>(UltimaIstruzioneSecondo);

  BranchSecondo->setSuccessor(0, SecondoLoop->getLoopLatch());
  BranchSecondo->setSuccessor(1, SecondoLoop->getLoopLatch());

  return true;
}

namespace { struct LoopFusion : PassInfoMixin<LoopFusion> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
      bool mod = false;      
      bool guardia;
      LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
      DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
      PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
      ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
      DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
      //Controllo due a due
      for (Loop *PrimoLoop: LI) {
        for (Loop *SecondoLoop: LI) {
          if (PrimoLoop == SecondoLoop) { continue; }
          // GUARDIA
          if (conGuardia(PrimoLoop, SecondoLoop)) {
            guardia = true;
          } else if (senzaGuardia(PrimoLoop, SecondoLoop)) {
            guardia = false;
          } else {
            errs () << "CON/SENZA GUARDIA HANNO ISTRUZIONI TRA LORO \n OPPURE LE GUARDIE HANNO COND DIVERSA\n"; 
            continue;
          }
          //ORA SO CHE PRIMO LOOP è TALE E SECONDO LOOP è TALE
          // EQUIVALENZA
          if (!equivalenzaControlFlow(PrimoLoop, SecondoLoop, DT, PDT, guardia)) {
            errs() << "NO EQUIVALENZA DEL CONTROL FLOW\n";
            continue;
          }
          // STESSO NUMERO DI ITERAZIONI
          if (!numeroIterazioniUguali(PrimoLoop, SecondoLoop, SE)) {
            errs() << "DIVERSO NUMERO DI ITERAZIONI\n";
            continue;
          }
          //DIPENDENZE A DISTANZA NEGATIVA 
          if (dipendenzeDistanzaNegativa(PrimoLoop, SecondoLoop, DI, SE)) {
            errs() << "HANNO DIPENDENZE A DISTANZA NEGATIVA\n";
            continue;
          }
          
          if (unisciLoop(PrimoLoop, SecondoLoop, guardia)) {
          mod = true;
          errs() << "LOOP FUSION EFFETTUATA\n";
          } else {
            errs() << "UNIONE LOOP FALLITA\n";
          }
        }
      }

      return mod ? PreservedAnalyses::none() : PreservedAnalyses::all();
      
    }

    static bool isRequired() { return true; } 
}; }

PassPluginLibraryInfo getLoopFusionPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LoopFusion", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "fake-loop-fusion") {
                    FPM.addPass(LoopFusion());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoopFusionPluginInfo();
}
