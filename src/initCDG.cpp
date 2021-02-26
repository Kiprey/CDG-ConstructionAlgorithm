//
// Created by wz on 2021/2/26.
//
#include "CDG.h"

using namespace SVF;
using namespace llvm;
using namespace std;
void findSSet(ICFGNode* entryNode,Set<const ICFGNode*> visited);
u32_t icfgOutEdgeNum(ICFGNode* iNode);
bool hasPostDom(DomTreeNodeBase<llvm::BasicBlock>* A,DomTreeNodeBase<llvm::BasicBlock>* B);

llvm::PostDominatorTree *PT= nullptr;
void CDG::initCDG(SVFFunction *fun ) {

    ICFG* icfg;
    PT->recalculate(*fun->getLLVMFun());
    ICFGNode* iNode= icfg->getFunEntryBlockNode(fun);
    ICFGNode* exitNode=icfg->getFunExitBlockNode(fun);

    Set<const ICFGNode*> visited;
    findSSet(iNode,visited);

}
void findSSet(ICFGNode* iNode,Set<const ICFGNode*> visited){
    if (visited.find(iNode) == visited.end())
        visited.insert(iNode);
    else
        return;
    if(SVFUtil::dyn_cast<FunExitBlockNode>(iNode))
        return;
    if(icfgOutEdgeNum(iNode)!=2){
        for (ICFGEdge* edge : iNode->getOutEdges())
            findSSet(edge->getDstNode(),visited);
        return;
    }
    //handle edges with condition
    for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
            iNode->OutEdgeEnd(); it != eit; ++it)
    {
        IntraCFGEdge* edge= SVFUtil::dyn_cast<IntraCFGEdge>(*it);
        if (edge)
            continue;
        findSSet(edge->getDstNode(),visited);
        NodeID TF = edge->getBranchCondtion().second;
        pair<IntraCFGEdge *, NodeID> S(edge, TF);
        const llvm::BasicBlock* bbA= edge->getSrcNode()->getBB();
        const llvm::BasicBlock* bbB= edge->getSrcNode()->getBB();
        DomTreeNodeBase<llvm::BasicBlock>* domNodeA=PT->getNode(bbA);
        DomTreeNodeBase<llvm::BasicBlock>* domNodeB=PT->getNode(bbB);
        if (!hasPostDom(domNodeA,domNodeB)){
            //@todo: 这里需要一个低成本的遍历方法找到从B到A父节点的路径
//            traveBackPostDom();
        }
    }
}
bool traveBackPostDom(DomTreeNodeBase<llvm::BasicBlock>* root){
    for (auto it=root->begin(),ie=root->end();it!=ie;++it);
    return true;
}


bool hasPostDom(DomTreeNodeBase<llvm::BasicBlock>* A,DomTreeNodeBase<llvm::BasicBlock>* B){
    for (auto it=B->begin(),ie=B->end();it!=ie;++it)
        if(B==A)
            return false;
    return true;
}

u32_t icfgOutEdgeNum(ICFGNode* iNode){
    u32_t n=0;
    for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
            iNode->OutEdgeEnd(); it != eit; ++it)
        ++n;
    return n;
}