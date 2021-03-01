//
// Created by wz on 2021/2/26.
//
#include "CDG.h"

using namespace SVF;
using namespace llvm;
using namespace std;

typedef DomTreeNodeBase<llvm::BasicBlock>* DTNodeTy;
typedef struct {
    DTNodeTy A;
    DTNodeTy B;
    DTNodeTy L;
    NodeID TF;
}DepenTupleTy;
typedef Set<DepenTupleTy*> DepenSSetTy;
typedef vector<DTNodeTy> DepenVecTy;

void findSSet(ICFGNode* entryNode,Set<const ICFGNode*> &visited,DepenSSetTy &setS);
void buildinitCDG(DepenSSetTy S);
u32_t icfgOutEdgeNum(ICFGNode* iNode);

void findPathA2B(DTNodeTy A,DepenSSetTy S,vector <DTNodeTy> &P);
void findPathL2B(DepenSSetTy S,vector <DTNodeTy> &P);
void handleDepenVec(DepenTupleTy* LB,vector <DTNodeTy> &P);

llvm::PostDominatorTree *PT= nullptr;

void CDG::initCDG(SVFFunction *fun ) {

    ICFG* icfg;
    PT->recalculate(*fun->getLLVMFun());

    const BasicBlock* exbb=SVFUtil::getFunExitBB(fun->getLLVMFun());
    const BasicBlock* enbb=&(fun->getLLVMFun()->front());

    Set<const ICFGNode*> visited;
    DepenSSetTy setS;
    findSSet(icfg->getFunEntryBlockNode(fun),visited,setS);
    buildinitCDG(setS);
}
void findSSet(ICFGNode* iNode,Set<const ICFGNode*> &visited,DepenSSetTy &setS){
    if (visited.find(iNode) == visited.end())
        visited.insert(iNode);
    else
        return;
    if(SVFUtil::dyn_cast<FunExitBlockNode>(iNode))
        return;
    if(icfgOutEdgeNum(iNode)!=2){
        for (ICFGEdge* edge : iNode->getOutEdges())
            findSSet(edge->getDstNode(),visited,setS);
        return;
    }
    //handle edges with condition to get S
    //traverse posdomtree to get P
    for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
            iNode->OutEdgeEnd(); it != eit; ++it)
    {
        IntraCFGEdge* edge= SVFUtil::dyn_cast<IntraCFGEdge>(*it);
        if (edge)
            continue;
        findSSet(edge->getDstNode(),visited,setS);
        NodeID TF = edge->getBranchCondtion().second;
        const llvm::BasicBlock* bbA= edge->getSrcNode()->getBB();
        const llvm::BasicBlock* bbB= edge->getSrcNode()->getBB();
        //判断改边的两个点是否存在支配关系，存在则存入S集合
        DTNodeTy dnA=PT->getNode(bbA);
        DTNodeTy dnB=PT->getNode(bbB);
        if (!PT->properlyDominates(dnA,dnB)){//Return true iff B dominates A and A != B.
            DTNodeTy dnL=PT->getNode(PT->findNearestCommonDominator(bbA,bbB));
            DepenTupleTy ABLT={dnA,dnB,dnL,TF};
            setS.insert(&ABLT);
        }
    }
}
void findPathA2B(DTNodeTy A,DepenSSetTy S,vector <DTNodeTy> &P){
    P.push_back(A);
    for(auto it=S.begin(),ie=S.end();it!=ie;it++){
        if(A==(*it)->B){
            handleDepenVec(*it,P);
            S.erase(it);
            break;
        }
    }
    for (auto it=A->begin(),ie=A->end();it!=ie;++it)
        findPathA2B(*it,S,P);
    P.pop_back();
}
void findPathL2B(const DepenSSetTy S,vector <DTNodeTy> &P){
    findPathA2B(PT->getRootNode(),S,P);
}

void buildinitCDG(DepenSSetTy S){
    vector <DTNodeTy> vecP;
    findPathL2B(S,vecP);
}
void handleDepenVec(DepenTupleTy* LB,vector <DTNodeTy> &P){
    DTNodeTy L=LB->L,B=LB->B,A=LB->A;
    auto pi=std::find(P.begin(),P.end(),L);//find L
    if(PT->properlyDominates(L,B))//if A is the ancestor of B,then A==L the path contant L,otherwise dont contain
        if (pi==P.end())
            return;
        else
            ++pi;
    for(;pi!=P.end();pi++){
        //@todo::添加节点和边
//        addNode((*pi)->getBlock());
//        addEdge(A,*(pi))
    }
}


u32_t icfgOutEdgeNum(ICFGNode* iNode){
    u32_t n=0;
    for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
            iNode->OutEdgeEnd(); it != eit; ++it)
        ++n;
    return n;
}