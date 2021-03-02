// #include "SVF-FE/LLVMUtil.h"
// #include "Graphs/SVFG.h"
// #include "WPA/Andersen.h"
// #include "SABER/LeakChecker.h"
// #include "SVF-FE/PAGBuilder.h"


#include "CDG.h"

using namespace SVF;
using namespace llvm;
using namespace std;

// static llvm::cl::opt<std::string> InputFilename(cl::Positional,
//         llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

// static llvm::cl::opt<bool> LEAKCHECKER("leak", llvm::cl::init(false),
//                                        llvm::cl::desc("Memory Leak Detection"));

ControlDependenceGraph* cdGraph = nullptr;

int main(int argc, char ** argv) {
    ControlDependenceGraph* cdGraph = new ControlDependenceGraph();
    // print CDG
    // delete node/edge/graph/PDT
    return 0;
}

