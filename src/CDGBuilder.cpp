//
// Created by wz on 2021/3/12.
//
#include "CDGBuilder.h"
#include "SVF-FE/LLVMUtil.h"

using namespace SVFUtil;

void CDGBuilder::build(SVFModule* svfModule){
    for(SVFModule::const_iterator iter=svfModule->begin(),eiter=svfModule->end();iter!=eiter;++iter){
        const SVFFunction* fun=*iter;
        if (SVFUtil::isExtCall(fun))
            continue;
        cdg->initCDG(fun);
        WorkList worklist;
        processCallsite(fun,worklist);
    }
}
/*!
 * function entry
 */
void CDGBuilder::processCallsite(const SVFFunction*  fun, WorkList& worklist)
{
    const Instruction* entryInst = &((fun->getLLVMFun()->getEntryBlock()).front());
    InstVec insts;
    if (isIntrinsicInst(entryInst))
        getNextInsts(entryInst, insts);
    else
        insts.push_back(entryInst);
    for (InstVec::const_iterator nit = insts.begin(), enit = insts.end();
         nit != enit; ++nit)
    {
        if(SVFUtil::isNonInstricCallSite(*nit)){
            handleCallSite(*nit);
        }
        worklist.push(*nit);
    }
    BBSet visited;
    while (!worklist.empty())
    {
        const Instruction* inst = worklist.pop();
        if (visited.find(inst) == visited.end())
        {
            visited.insert(inst);
            if (isReturn(inst))
                continue;
            InstVec nextInsts;
            getNextInsts(inst, nextInsts);
            for (InstVec::const_iterator nit = nextInsts.begin(), enit =
                    nextInsts.end(); nit != enit; ++nit)
            {
                const Instruction* succ = *nit;
                handleCallSite(succ);
                if (isNonInstricCallSite(inst))
                {
                    handleCallSite(*nit);
                }
                worklist.push(succ);
            }
        }
    }
}
void CDGBuilder::addInterCDGEdge(CDGNode* srcNode,CDGNode* dstNode){
    cdg->addCDGEdge(srcNode,dstNode,CDGEdge::LabelType_Default);
}


void CDGBuilder::handleCallSite(const Instruction *inst){
    assert(SVFUtil::isCallSite(inst) && "not a call instruction?");
    assert(SVFUtil::isNonInstricCallSite(inst) && "associating an intrinsic debug instruction with an ICFGNode!");
    CDGNode* srcNode = getOrAddControlCDGNode(inst->getParent());
    if (const SVFFunction*  callee = getCallee(inst)){
        CDGNode* dstNode = getOrAddControlCDGNode(&(callee->getLLVMFun()->getEntryBlock()));
        addInterCDGEdge(srcNode, dstNode);                       //creating interprocedural edges
    }
}
CDGNode* CDGBuilder::getOrAddControlCDGNode(const llvm::BasicBlock* bb){
    if(NodeID cNodeID=cdg->getNodeIDFromBB(bb)==-1){
        ControlCDGNode* cNode =cdg->addControlCDGNode(bb);
        return cNode;
    }
    else{
        return cdg->getCDGNode(cNodeID);
    }
}