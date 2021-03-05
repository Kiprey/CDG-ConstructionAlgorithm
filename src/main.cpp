// #include "SVF-FE/LLVMUtil.h"
// #include "Graphs/SVFG.h"
// #include "WPA/Andersen.h"
// #include "SABER/LeakChecker.h"
#include "SVF-FE/PAGBuilder.h"


#include "CDG.h"

using namespace SVF;
using namespace llvm;
using namespace std;

// static llvm::cl::opt<std::string> InputFilename(cl::Positional,
//         llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

// static llvm::cl::opt<bool> LEAKCHECKER("leak", llvm::cl::init(false),
//                                        llvm::cl::desc("Memory Leak Detection"));


static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

int main(int argc, char ** argv) {
    int arg_num = 0;
    char **arg_value = new char*[argc];
    std::vector<std::string> moduleNameVec;
    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Whole Program Points-to Analysis\n");

    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (PAG)
    PAGBuilder builder;
    PAG* pag = builder.build(svfModule);
    /// ICFG
    const SVFFunction* fun;
    NodeID n=0;
    for (auto it=svfModule->begin(),ie=svfModule->end();it!=ie;it++,n++){
        outs()<<"FuncName::"<<(*it)->getName()<<"\n";
        if(n==0){
            fun=*it;
            break;
        }
    }
    ICFG* icfg = pag->getICFG();
    ControlDependenceGraph* cdGraph = new ControlDependenceGraph(icfg);
    cdGraph->initCDG(fun);
    // print CDG
    // delete node/edge/graph/PDT
//    icfg->dump("wz_icfg");
    return 0;
}

