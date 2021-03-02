#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph()
{
    /// @todo 构建PDT
}

/*!
 * 构建初始的CDG图
 * @param fun
 */
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
/*!
 * 遍历CFG的所有分支节点，找到S集合存入set'S中
 * @param iNode CFG节点
 * @param visited 记录已访问的CFG节点
 * @param setS S集合（A,B,L,TF）
 */
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

/*!
 * 根据给出的S集合找到后支配树上的路径，也就是A的CD集
 * @param S S集合
 */
void CDG::buildinitCDG(DepenSSetTy S){
    vector <DTNodeTy> vecP;
    findPathL2B(S,vecP);
}

/*!
 * 先根遍历后支配树，记录S集合中从L到B的路径
 * @param S S集合
 * @param P 记录路径
 */
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

/*!
 * 处理路径P生成CD集
 * @param LB
 * @param P
 */
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
    CDGEdge::LabelType l=lable2bool(TF);
    CDSetElemTy tmpCDSetElem(nodeA->getId(),l);
    for(;pi!=P.end();pi++){//遍历路径P，添加节点和边
        CDGNode* CDnode=addControlCDGNode((*pi)->getBlock());
        addCDGEdge(nodeA,CDnode,l);
        //得到依赖集（CD集）
        auto iter=CDMap.find(nodeA->getId());
        if(iter==CDMap.end()) {//没找到就就，新一个集合
            CDSetTy tmpCDSet;
            tmpCDSet.insert(tmpCDSetElem);
            CDMap.insert({nodeA->getId(),tmpCDSet});
        }
        else{//添加被依赖节点A到依赖的依赖集里
            (*iter).second.insert(tmpCDSetElem);
        }
    }
}

/*!
 * 将u32的lable类型转为CDGEdge::LableType类型
 * @param TF
 * @return
 */
CDGEdge::LabelType CDG::lable2bool(NodeID TF){
    switch (TF) {
        case 0:
            return CDGEdge::LabelType::F;
        case 1:
            return CDGEdge::LabelType::T;
        default:
            return CDGEdge::LabelType::None;
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

inline void ControlDependenceGraph::addCDGEdge(CDGNode* s,CDGNode* d,CDGEdge::LabelType l)
{
    CDGEdge* edge=new CDGEdge(s,d,l);
    bool added1 = edge->getDstNode()->addIncomingEdge(edge);
    bool added2 = edge->getSrcNode()->addOutgoingEdge(edge);
}

inline CDGNode* ControlDependenceGraph::getCDGNode(NodeID id)
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