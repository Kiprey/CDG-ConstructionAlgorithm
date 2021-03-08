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

static llvm::cl::opt<std::string> InputFuncName("cdgfun",  llvm::cl::init("main"),
                                                llvm::cl::desc("Please specify which function needs to be build cdg"));

//static llvm::cl::opt<string> InputFuncName("read-ander",  llvm::cl::init(""),
//                                       llvm::cl::desc("Read Andersen's analysis results from a file"));
int main(int argc, char **argv)
{
    int arg_num = 0;
    char **arg_value = new char *[argc];
    std::vector<std::string> moduleNameVec;

    outs()<<"handle fun:"<<InputFuncName<<"\n";
    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Whole Program Points-to Analysis\n");

    SVFModule *svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (PAG)
    PAGBuilder builder;
    PAG *pag = builder.build(svfModule);
    /// ICFG
    const SVFFunction *fun;
    bool isFindInputFun = false;
    for (auto it = svfModule->begin(), ie = svfModule->end(); it != ie; it++)
    {
        outs() << "FuncName::" << (*it)->getName() << "\n";
        if ((*it)->getName().compare(InputFuncName)==0)
        {
            fun = *it;
            isFindInputFun=true;
            break;
        }
    }
    if(isFindInputFun==false) {
        outs()<<"cant find input fun:"<<InputFuncName;
        return 0;
    }
    ICFG *icfg = pag->getICFG();
    ControlDependenceGraph *cdGraph = new ControlDependenceGraph(icfg);
    cdGraph->initCDG(fun);
    // print CDG
    // delete node/edge/graph/PDT
    //    icfg->dump("wz_icfg");
    
    return 0;
}
