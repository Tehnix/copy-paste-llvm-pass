#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include <unordered_map>
#include <vector>
using namespace llvm;

namespace {

  // Container for all the location metadata
  struct LocationMetadata {
    std::string  functionName;
    unsigned int lineNumber;
    unsigned int colNumber;
    StringRef    fileName;
    StringRef    dirName;
    unsigned int ifBranchNo;
    unsigned int elseBranchNo;
  };

  // Container for the call metadata
  struct CallMetadata {
    LocationMetadata loc;
    Function         *fun;
    CallInst         *callInst;
  };

  // Container for the assignment metadata
  struct AssignmentMetadata {
    LocationMetadata loc;
    StringRef        lhs;
  };

  // Keep track of all the call instructions
  std::unordered_map<std::string, std::vector<CallMetadata>>
    callMap = {};
  // Keep track of all the integer assignment instructions
  std::unordered_map<int64_t, std::vector<AssignmentMetadata>>
    intAssignmentMap = {};

  // Convert a call instruction into metadata and insert it into the
  // callMap.
  void instructionToCallMap(Instruction &I, LocationMetadata loc) {
    // Look for a call instruction to catch function calls.
    if (CallInst *callInst = dyn_cast<CallInst>(&I)) {
      // Get the  pointer of the function that is called.
      Function *fun = callInst->getCalledFunction();
      // Ignore the clang debug functions
      if (fun->getName().str() != "llvm.dbg.declare") {
        // Make sure the key exists in the callMap.
        if (callMap.find(fun->getName().str()) == callMap.end()) {
          callMap[fun->getName().str()] = {};
        }
        // Extract the relevant data and save it.
        CallMetadata meta = {loc, fun, callInst};
        callMap[fun->getName().str()].push_back(meta);
      }
    }
  };

  // Convert a store instruction into metadata and insert it into the
  // intAssignmentMap.
  void instructionToIntAssignmentMap(Instruction &I, LocationMetadata loc) {
    // Look for a store instruction to catch assignments.
    if (StoreInst *storeInst = dyn_cast<StoreInst>(&I)) {
      // Check that the allocation is not an argument to a function, which
      // always has '.addr' appended to it.
      StringRef varName = storeInst->getPointerOperand()->getName();
      if (!varName.endswith(".addr")) {
        AssignmentMetadata meta = {loc, varName};
        Value *val = storeInst->getValueOperand();
        // Get the integer value of the assignment.
        if (ConstantInt *constInt = dyn_cast<ConstantInt>(val)) {
          intAssignmentMap[constInt->getSExtValue()].push_back(meta);
        }
      }
    }
  };

  // Check if the arguments to two function calls are identical. Limited to
  // integer arguments.
  bool compareIntArguments(CallInst *callInst1, CallInst *callInst2) {
    unsigned int noOfArgs1 = callInst1->getNumArgOperands();
    unsigned int noOfArgs2 = callInst2->getNumArgOperands();
    // First, check that the number of arguments match
    if (noOfArgs1 != noOfArgs2) {
      return false;
    }
    unsigned int numberOfEqualArguments = 0;
    for (int i = 0; i < noOfArgs1; i++) {
      Value *val1 = callInst1->getArgOperand(i);
      Value *val2 = callInst2->getArgOperand(i);
      // Compare integer values
      if (ConstantInt *constInt1 = dyn_cast<ConstantInt>(val1)) {
        if (ConstantInt *constInt2 = dyn_cast<ConstantInt>(val2)) {
          if (constInt1->getSExtValue() == constInt2->getSExtValue()) {
            numberOfEqualArguments++;
          }
        }
      }
    }
    return numberOfEqualArguments == noOfArgs1;
  };

  // Check if the arguments to two function calls are similar. Limited to
  // integer arguments.
  // By similar the requirements are that the function has at least 3 arguments
  // and at least 2 of them are identical.
  bool compareSimilarIntArguments(CallInst *callInst1, CallInst *callInst2) {
    unsigned int noOfArgs1 = callInst1->getNumArgOperands();
    unsigned int noOfArgs2 = callInst2->getNumArgOperands();
    // First, check that the number of arguments match and that we have at least
    // 3 arguments in the call (similarity requirement).
    if (noOfArgs1 != noOfArgs2 && noOfArgs1 < 3) {
      return false;
    }
    unsigned int numberOfEqualArguments = 0;
    for (int i = 0; i < noOfArgs1; i++) {
      Value *val1 = callInst1->getArgOperand(i);
      Value *val2 = callInst2->getArgOperand(i);
      // Compare integer values
      if (ConstantInt *constInt1 = dyn_cast<ConstantInt>(val1)) {
        if (ConstantInt *constInt2 = dyn_cast<ConstantInt>(val2)) {
          if (constInt1->getSExtValue() == constInt2->getSExtValue()) {
            numberOfEqualArguments++;
          }
        }
      }
    }
    return numberOfEqualArguments >= 2;
  };

  // Base class for implementing a detection.
  class Detection {
  public:
    virtual void parseMetadata() {}
  };

  class DetectExtractIntAssignmentToConstant: public Detection {
  public:
    void parseMetadata(int64_t key, std::vector<AssignmentMetadata> metaVector) {
      // Check if there are more than one declaration of the same value.
      if (metaVector.size() > 1) {
        errs() << "Saw several assignments for '" << key << "' to\n";
        // Go through the metadata for each of the calls and notify about all
        // the cases where the variable is declared.
        for (auto meta : metaVector) {
          errs() << "    Variable '" << meta.lhs << "' at line " <<
                    meta.loc.lineNumber << ", column " << meta.loc.colNumber <<
                    " in function '" << meta.loc.functionName << "' in " << meta.loc.fileName << "\n";
        }
        errs() << "Consider moving it to a constant.\n";
      }
    }
  } detectExtractIntAssignmentToConstant;

  class DetectUnnecessaryIfThenElse: public Detection {
  public:
    void parseMetadata(std::string key, std::vector<CallMetadata> metaVector) {
      // The case is only relevant when there are multiple calls to the same
      // function.
      if (metaVector.size() > 1) {
        // Go through the metadata for each of the calls
        unsigned int metaSize = metaVector.size();
        for (int c = 0; c < metaSize-1; c++) {
          auto meta = metaVector[c];
          // If we are in an if-branch, check the rest of the calls for a
          // matching else-branch
          if (meta.loc.ifBranchNo > 0) {
            bool foundIdenticalCall = false;
            for (int cr = c+1; cr < metaSize; cr++) {
              auto rMeta = metaVector[cr];
              if (rMeta.loc.functionName == meta.loc.functionName &&
                  rMeta.loc.elseBranchNo == meta.loc.ifBranchNo &&
                  compareIntArguments(rMeta.callInst, meta.callInst)) {
                foundIdenticalCall = true;
              }
              if (foundIdenticalCall) {
                errs() << "Saw call to '" << key << "' in function '" <<
                          meta.loc.functionName << "' in both an if-branch " <<
                          "and an else-branch of the same if-then-else\n";
                errs() << "    Called at line " << meta.loc.lineNumber <<
                          ", column " << meta.loc.colNumber << " in " <<
                          meta.loc.fileName << "\n";
                errs() << "    Called at line " << rMeta.loc.lineNumber <<
                          ", column " << rMeta.loc.colNumber << " in " <<
                          rMeta.loc.fileName << "\n";
              }
            }
          }
        }
      }
    }
  } detectUnnecessaryIfThenElse;

  class DetectSimilarFunctionCall: public Detection {
  public:
    void parseMetadata(std::string key, std::vector<CallMetadata> metaVector) {
      // The case is only relevant when there are multiple calls to the same
      // function.
      if (metaVector.size() > 1) {
        // Go through the metadata for each of the calls
        unsigned int metaSize = metaVector.size();
        for (int c = 0; c < metaSize-1; c++) {
          auto meta = metaVector[c];
          // Check the remaining calls for a matching call
          for (int cr = c+1; cr < metaSize; cr++) {
            bool foundIdenticalCall = false;
            auto rMeta = metaVector[cr];
            if (compareSimilarIntArguments(rMeta.callInst, meta.callInst)) {
              foundIdenticalCall = true;
            }
            if (foundIdenticalCall) {
              errs() << "Saw similar calls to '" << key << "'\n";
              errs() << "    Called at line " << meta.loc.lineNumber <<
                        ", column " << meta.loc.colNumber << " in " <<
                        meta.loc.fileName << "\n";
              errs() << "    Called at line " << rMeta.loc.lineNumber <<
                        ", column " << rMeta.loc.colNumber << " in " <<
                        rMeta.loc.fileName << "\n";
            }
          }
        }
      }
    }
  } detectSimilarFunctionCall;

  class DetectIdenticalFunctionCall: public Detection {
  public:
    void parseMetadata(std::string key, std::vector<CallMetadata> metaVector) {
      // The case is only relevant when there are multiple calls to the same
      // function.
      if (metaVector.size() > 1) {
        // Go through the metadata for each of the calls
        unsigned int metaSize = metaVector.size();
        for (int c = 0; c < metaSize-1; c++) {
          auto meta = metaVector[c];
          // Check the remaining calls for a matching call
          for (int cr = c+1; cr < metaSize; cr++) {
            bool foundIdenticalCall = false;
            auto rMeta = metaVector[cr];
            if (compareIntArguments(rMeta.callInst, meta.callInst)) {
              foundIdenticalCall = true;
            }
            if (foundIdenticalCall) {
              errs() << "Saw identical calls to '" << key << "'\n";
              errs() << "    Called at line " << meta.loc.lineNumber <<
                        ", column " << meta.loc.colNumber << " in " <<
                        meta.loc.fileName << "\n";
              errs() << "    Called at line " << rMeta.loc.lineNumber <<
                        ", column " << rMeta.loc.colNumber << " in " <<
                        rMeta.loc.fileName << "\n";
            }
          }
        }
      }
    }
  } detectIdenticalFunctionCall;

  // The actual LLVM pass
  struct CopyPasteDetectionPass : public FunctionPass {
    static char ID;
    CopyPasteDetectionPass() : FunctionPass(ID) {}

    // Used for the parsing phase
    virtual bool runOnFunction(Function &F) {
      for (auto &B : F) {
        for (auto &I : B) {
          // Get the debug information (requires the -g flag with clang).
          unsigned int lineNumber = false;
          unsigned int colNumber = false;
          StringRef fileName = "";
          StringRef dirName = "";
          // Yet to be implemented
          unsigned int ifBranchNo = false;
          unsigned int elseBranchNo = false;
          if (DILocation *loc = I.getDebugLoc()) {
            lineNumber = loc->getLine();
            colNumber = loc->getColumn();
            fileName = loc->getFilename();
            dirName = loc->getDirectory();
          }
          LocationMetadata loc = {
            F.getName().str(),
            lineNumber,
            colNumber,
            fileName,
            dirName,
            ifBranchNo,
            elseBranchNo
          };
          // Inspect the instruction and add it to the appropriate map
          instructionToCallMap(I, loc);
          instructionToIntAssignmentMap(I, loc);
        }
      }
      return false; // Since we didn't modify the code, we return false
    }

    // Used for the detection phase
    virtual bool doFinalization(Module &M) {
      // Go through all the function calls
      for (auto c : callMap) {
        detectUnnecessaryIfThenElse.parseMetadata(
          c.first,
          c.second
        );
        detectIdenticalFunctionCall.parseMetadata(
          c.first,
          c.second
        );
        detectSimilarFunctionCall.parseMetadata(
          c.first,
          c.second
        );
      }
      // Go through all the int assignments
      for (auto a : intAssignmentMap) {
        detectExtractIntAssignmentToConstant.parseMetadata(
          a.first,
          a.second
        );
      }
      return false; // Since we didn't modify the code, we return false
    }

  };
}


// Automatically enable the pass, courtesy of Adrian Sampson,
// http://adriansampson.net/blog/clangpass.html
char CopyPasteDetectionPass::ID = 0;
static void registerCopyPasteDetectionPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
  PM.add(new CopyPasteDetectionPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerCopyPasteDetectionPass);
