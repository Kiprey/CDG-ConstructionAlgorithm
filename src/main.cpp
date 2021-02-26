// #include "SVF-FE/LLVMUtil.h"
// #include "Graphs/SVFG.h"
// #include "WPA/Andersen.h"
// #include "SABER/LeakChecker.h"
// #include "SVF-FE/PAGBuilder.h"


#include "CDG.h"
#include "CDmap.h"

using namespace SVF;
using namespace llvm;
using namespace std;

// static llvm::cl::opt<std::string> InputFilename(cl::Positional,
//         llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

// static llvm::cl::opt<bool> LEAKCHECKER("leak", llvm::cl::init(false),
//                                        llvm::cl::desc("Memory Leak Detection"));

ControlDependenceGraph* CDG = nullptr;
PostDominatorTree* PDT = nullptr;

int main(int argc, char ** argv) {
    // 1. 生成 initial CDG
    // 2. 生成后向支配树
    // 3. 插入RegionNode
    
    return 0;
}

