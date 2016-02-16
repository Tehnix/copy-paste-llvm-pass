#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

namespace {
  struct CountPrintf : public FunctionPass {
    static char ID;
    CountPrintf() : FunctionPass(ID) {}

    // Runs on every function call, independently
    virtual bool runOnFunction(Function &F) {
      errs() << "Inside of function '" << F.getName() << "'\n";
      for (auto &B : F) {
        for (auto &I : B) {
          // Look for a call instruction, returning NULL if the cast fails
          if (CallInst *callInst = dyn_cast<CallInst>(&I)) {
            // Get the function pointer of the function that is called
            Function *f = callInst->getCalledFunction();
            errs() << "    Saw call for: " << f->getName() << "\n";
          }
        }
      }
      // Since we didn't modify the code, we return false
      return false;
    }
    
  };
}

char CountPrintf::ID = 0;

// Automatically enable the pass, curtesy of Adrian Sampson,
// http://adriansampson.net/blog/clangpass.html
static void registerCountPrintf(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new CountPrintf());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerCountPrintf);
