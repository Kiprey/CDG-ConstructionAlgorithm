#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph()
{
    /// @todo
}

void ControlDependenceGraph::initCDG(SVFFunction *fun )
{
    ICFG* icfg;
    PDT->recalculate(*fun->getLLVMFun());

    const BasicBlock* exbb=SVFUtil::getFunExitBB(fun->getLLVMFun());
    const BasicBlock* enbb=&(fun->getLLVMFun()->front());

    Set<const ICFGNode*> visited;
    DepenSSetTy setS;
    findSSet(icfg->getFunEntryBlockNode(fun),visited,setS);
    buildinitCDG(setS);
}

void CDG::findSSet(ICFGNode* iNode,Set<const ICFGNode*> &visited,DepenSSetTy &setS){
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
        DTNodeTy dnA=PDT->getNode(bbA);
        DTNodeTy dnB=PDT->getNode(bbB);
        if (!PDT->properlyDominates(dnA,dnB)){//Return true iff B dominates A and A != B.
            DTNodeTy dnL=PDT->getNode(PDT->findNearestCommonDominator(bbA,bbB));
            DepenTupleTy ABLT={dnA,dnB,dnL,TF};
            setS.insert(&ABLT);
        }
    }
}

void CDG::findPathL2B(const DepenSSetTy S,vector <DTNodeTy> &P){
    findPathA2B(PDT->getRootNode(),S,P);
}

void CDG::findPathA2B(DTNodeTy A,DepenSSetTy S,vector <DTNodeTy> &P){
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

void CDG::buildinitCDG(DepenSSetTy S){
    vector <DTNodeTy> vecP;
    findPathL2B(S,vecP);
}
void CDG::handleDepenVec(DepenTupleTy* LB,vector <DTNodeTy> &P){
    DTNodeTy L=LB->L,B=LB->B,A=LB->A;
    NodeID TF=LB->TF;
    CDGNode* nodeA=addControlCDGNode(A->getBlock());//all node below dependent on A
    auto pi=std::find(P.begin(),P.end(),L);//find L
    if(PDT->properlyDominates(L,B))//if A is the ancestor of B,then A==L the path contant L,otherwise dont contain
        if (pi==P.end())
            return;
        else
            ++pi;
    for(;pi!=P.end();pi++){
        CDGNode* CDnode=addControlCDGNode((*pi)->getBlock());
        switch (TF) {
            case 0:
                addCDGEdge(nodeA,CDnode,CDGEdge::LabelType::T);
                break;
            case 1:
                addCDGEdge(nodeA,CDnode, CDGEdge::LabelType::F);
                break;
            default:
                addCDGEdge(nodeA,CDnode,CDGEdge::LabelType::None);
        }
    }
}

u32_t CDG::icfgOutEdgeNum(ICFGNode* iNode){
    u32_t n=0;
    for (ICFGNode::const_iterator it = iNode->OutEdgeBegin(), eit =
            iNode->OutEdgeEnd(); it != eit; ++it)
        ++n;
    return n;
}

inline void ControlDependenceGraph::addCDGNode(CDGNode* node)
{
    _bb2CDGNodeID.insert(make_pair(node->getBasicBlock(),node->getId()));
    addGNode(node->getId(), node);
}
inline CDGNode* ControlDependenceGraph::addControlCDGNode(BasicBlock* nbb)
{
    CDGNode* iNode=new CDGNode(CDGNode::ControlNode);
    addCDGNode(iNode);
    return iNode;
}
inline CDGNode* ControlDependenceGraph::addRegionCDGNode()
{
    CDGNode* iNode=new CDGNode(CDGNode::RegionNode);
    addCDGNode(iNode);
    return iNode;
}

inline void CDG::addCDGEdge(CDGNode* s,CDGNode* d,CDGEdge::LabelType l)
{
    CDGEdge* edge=new CDGEdge(s,d,l);
    bool added1 = edge->getDstNode()->addIncomingEdge(edge);
    bool added2 = edge->getSrcNode()->addOutgoingEdge(edge);
}

CDGNode* ControlDependenceGraph::getCDGNode(NodeID id)
{
    return getGNode(id);
}
bool ControlDependenceGraph::hasCDGNode(NodeID id)
{
    return hasGNode(id);
}
void ControlDependenceGraph::removeCDGNode(CDGNode* node)
{
    _bb2CDGNodeID.erase(node->getBasicBlock());
    removeGNode(node);
}

NodeID ControlDependenceGraph::getNodeIDFromBB(BasicBlock* bb)
{
    auto iter = _bb2CDGNodeID.find(bb);
    if(iter != _bb2CDGNodeID.end())
        return iter->second;
    else
        return -1;
}

ControlDependenceEdge::ControlDependenceEdge(
        ControlDependenceNode* s, 
        ControlDependenceNode* d, 
        LabelType k)  
    : GenericCDEdgeTy(s,d,k) {}

NodeID ControlDependenceNode::_nextNodeID = 0;

ControlDependenceNode::ControlDependenceNode(NodeType ty)
    :  GenericCDNodeTy(_nextNodeID++, ty) {};

void ControlDependenceNode::setBasicBlock(BasicBlock* bb)
{
    _bb = bb;
}

BasicBlock* ControlDependenceNode::getBasicBlock()
{
    return _bb;
}