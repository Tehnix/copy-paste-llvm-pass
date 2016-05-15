#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <unordered_map>
#include <vector>
using namespace llvm;


struct CallMetadata {
  Function::ArgumentListType arguments;
  std::string functionName;
};

struct VariableMetadata {
  // Value rhs;
  std::string functionName;
};

// Keep track of all the call instructions
std::unordered_map<std::string, std::vector<CallMetadata>> callInstMap = {};
// Keep track of all the variable instructions
std::unordered_map<std::string, std::vector<VariableMetadata>> variableInstMap = {};

namespace {
  struct DuplicationDetectionPass : public FunctionPass {
    static char ID;
    DuplicationDetectionPass() : FunctionPass(ID) {}

    // Runs on every function call, independently
    virtual bool runOnFunction(Function &F) {
      for (auto &B : F) {
        for (auto &I : B) {
          // Look for a call instruction, returning NULL if the cast fails
          if (CallInst *callInst = dyn_cast<CallInst>(&I)) {
            // Get the function pointer of the function that is called
            Function *fun = callInst->getCalledFunction();
            // Check if the key exists, else add it first
            if (callInstMap.find(fun->getName().str()) == callInstMap.end()) {
              callInstMap[fun->getName().str()] = {};
            } else {
              // This should also compare the arguments to the function
              errs() << "Have already seen a call to " << fun->getName() << " before!\n";
            }
            CallMetadata meta = {
              fun->getArgumentList(),
              F.getName().str()
            };
            callInstMap[fun->getName().str()].push_back(meta);
          }
        }
      }
      // Since we didn't modify the code, we return false
      return false;
    }

    virtual bool doFinalization(Module &M) {
      // Go through all the function calls
      for (auto c : callInstMap) {
        auto callMetadata = c.second;
        // Go through the metadata for each of the calls
        for (auto meta : callMetadata) {
          errs() << "Saw " << c.first << " inside function " << meta.functionName << "\n";
        }
      }
      // Since we didn't modify the code, we return false
      return false;
    }

  };
}

char DuplicationDetectionPass::ID = 0;

// Automatically enable the pass, courtesy of Adrian Sampson,
// http://adriansampson.net/blog/clangpass.html
static void registerDuplicationDetectionPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new DuplicationDetectionPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerDuplicationDetectionPass);
