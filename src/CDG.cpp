#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph(ICFG *icfg) : icfg(icfg), totalCDGNode(0)
{
    PDT = new llvm::PostDominatorTree();
}
/**
 *
 */
void CDG::buildSVFDG() {
    const SVFFunction *fun;
    //遍历函数的每个StmtSVFG节点
    for(auto node:svfg->getVFGNodes(fun)){
        if(StmtSVFGNode* node=SVFUtil::dyn_cast<StmtSVFGNode>(node)){
            const BasicBlock* bb=node->getInst()->getParent();
            auto cditer=CDMap.find(getNodeIDFromBB(bb));
            if(cditer==CDMap.end() )
                return;
            CDSetTy cdSet=cditer->second;
            for
        }
    }
    for(auto iter=CDMap.begin(),eiter=CDMap.end();iter!=eiter;++iter){
        const BasicBlock* curbb=getGNode(iter->first)->getBB();
        for(auto& inst: *curbb){

        }
    }


}















/**
 * 构建后支配树，添加一个before_entry节点
 */
void CDG::buildPDT(const SVFFunction *fun, DepenSSetTy &setS)
{
    llvm::BasicBlock *entryBB = &(fun->getLLVMFun()->front());
    llvm::BasicBlock *exitBB = &(fun->getLLVMFun()->back());
    PDT->recalculate(*fun->getLLVMFun()); //1.初始化后支配树
    //2.创建一个before_entry节点，将它放到后置支配树上根节点后面，这并不表示后支配关系，而是为了后面方便处理
    BasicBlock *before_entry = llvm::BasicBlock::Create(entryBB->getContext(), "before_entry", fun->getLLVMFun(),
                                                        exitBB);
    ///@warning 这里注意exit节点存储的的基本块指针为Null
    DTNodeTy dnA = PDT->addNewBlock(before_entry, exitBB);
    DTNodeTy dnB = PDT->getNode(entryBB);
    DTNodeTy dnL = PDT->getRootNode(); //就是exitBB
    PDT->print(outs());
    DepenTupleTy ABLT = {dnA, dnB, dnL, 0}; //3.在S集合中添加一个元素，一个四元组表示before的依赖关系
    setS.insert(ABLT);
}

/*!
 * 构建初始的CDG图
 * @param fun
 */
void ControlDependenceGraph::initCDG(const SVFFunction *fun)
{
    //1.初始化PDT和S集合
    DepenSSetTy setS;
    buildPDT(fun, setS);

    //2.为每个PDT节点初始化一个CDG节点
    initCDGNodeFromPDTRoot(fun,PDT);

    //3.找到S集合
    Set<const ICFGNode *> visited;
    findSSet(icfg->getFunEntryBlockNode(fun), visited, setS);
    showSetS(setS, outs());

    //4.根据S集合计算依赖关系，同时添加节点和边构建初始的CDG
    buildinitCDG(setS);
    showCDMap(CDMap, outs());
    dump("cdg_initial");

    //5.添加RegionNode
    addRegionNodeToCDG();
    dump("cdg_final");
}

/*!
 * 遍历CFG的所有分支节点，找到S集合存入set'S中
 * @param iNode CFG节点
 * @param visited 记录已访问的CFG节点
 * @param setS S集合（A,B,L,TF）
 */
void CDG::findSSet(ICFGNode *iNode, Set<const ICFGNode *> &visited, DepenSSetTy &setS)
{
    if (visited.find(iNode) != visited.end())
        return;
    visited.insert(iNode);
    if (!(iNode->hasOutgoingEdge()) || SVFUtil::dyn_cast<FunExitBlockNode>(iNode))
        return;
    /// @warning 注意这里的判断条件，即便只有一个出边，br也可能不为空!(br&&br->isConditional()) ||icfgOutIntraEdgeNum(iNode)==1
    ///大于1，且没有br是可以的，比如switch
    /// @todo 可能爆栈
    //cond1. 这里处理没有条件分支的节点，也就是只有一个过程内出边的节点，entry节点是特殊情况
    if (icfgOutIntraEdgeNum(iNode) == 1 || SVFUtil::isa<FunEntryBlockNode>(iNode))
    {
        for (ICFGEdge *edge : iNode->getOutEdges())
            if (SVFUtil::isa<IntraCFGEdge>(edge)) //only handle intraCFGedge
                findSSet(edge->getDstNode(), visited, setS);
        return;
    }
    //cond2. handle edges with condition to get S
    for (ICFGEdge *edge : iNode->getOutEdges())
    {
        IntraCFGEdge *intraEdge = SVFUtil::dyn_cast<IntraCFGEdge>(edge);
        if (intraEdge == nullptr)
            continue;
        findSSet(intraEdge->getDstNode(), visited, setS);
        //判断该边的两个点是否存在支配关系，存在则存入S集合
        int l = intraEdge->getBranchCondtion().second;
        const llvm::BasicBlock *bbA = intraEdge->getSrcNode()->getBB();
        const llvm::BasicBlock *bbB = intraEdge->getDstNode()->getBB();
        const SwitchInst *cbr = dyn_cast<SwitchInst>(&(iNode->getBB()->back()));
        if (cbr)
        {
            if (bbB == cbr->getDefaultDest())
                l = CDGEdge::LabelType_Default;
            else
                for (auto Case : cbr->cases())
                {
                    if (Case.getCaseSuccessor() != bbB)
                    {
                        l = Case.getCaseIndex();
                        break;
                    }
                }
        }
        DTNodeTy dnA = PDT->getNode(bbA);
        DTNodeTy dnB = PDT->getNode(bbB);
        if (!PDT->properlyDominates(dnB, dnA))
        { //Return true iff B dominates A and A != B.
            DTNodeTy dnL = PDT->getNode(PDT->findNearestCommonDominator(bbA, bbB));
            DepenTupleTy ABLT = {dnA, dnB, dnL, l};
            setS.insert(ABLT);
        }
    }
}

/*!
 * 根据给出的S集合找到后支配树上的路径，也就是A的CD集
 * @param S S集合
 */
void CDG::buildinitCDG(DepenSSetTy S)
{
    vector<DTNodeTy> vecP;
    findPathL2B(S, vecP);
}

/*!
 * 先根遍历后支配树，记录S集合中从L到B的路径,当遍历到某个S集合中的B节点时，回溯路径记录CD集
 * @param S S集合
 * @param P 记录路径
 */
void CDG::findPathL2B(DepenSSetTy &S, vector<DTNodeTy> &P)
{
    outs() << "Traversing the DomTree From Root"
           << "\n"; //DEBUG
    findPathA2B(PDT->getRootNode(), S, P);
}

void CDG::findPathA2B(DTNodeTy A, DepenSSetTy &S, vector<DTNodeTy> &P)
{
    outs() << "Traver Dom_node_value: " << A; //DEBUG
    P.push_back(A);
    for (auto it = S.begin(), ie = S.end(); it != ie; it++)
    {
        if (A == it->B)
        {
            outs() << "Find B!!"
                   << "\n";         //DEBUG
            handleDepenVec(*it, P); //回溯
            S.erase(it);
            break;
        }
    }
    for (auto it = A->begin(), ie = A->end(); it != ie; ++it)
        findPathA2B(*it, S, P);
    P.pop_back();
}

/**
 * 为每个PDT节点初始化CDG节点，注意根据PDT中exit初始化的节点的basicblock为Null
 * @param dtNode PDT节点
 */
void CDG::initCDGNodeFromPDTRoot(const SVFFunction *fun,PostDominatorTree* pdt)
{
    DTNodeTy dtNode=pdt->getRootNode();
    ControlCDGNode* cNode =addControlCDGNode(dtNode->getBlock());
    FunToFunBeforeEntryNodeMap[fun]=cNode;
    for (auto it = dtNode->begin(), ie = dtNode->end(); it != ie; ++it)
        initCDGNodeFromPDT(*it);
}

void CDG::initCDGNodeFromPDT(DTNodeTy dtNode)
{
    addControlCDGNode(dtNode->getBlock());
    for (auto it = dtNode->begin(), ie = dtNode->end(); it != ie; ++it)
        initCDGNodeFromPDT(*it);
}

/*!
 * 回溯P，将路径中从L开始到B结束的节点，这些节点都依赖A
 * 如果L==A，那么（依赖A的节点）节点集就包括L（A）
 * 如果L！=A，节点集就不包括L
 * @param LB
 * @param P
 */
void CDG::handleDepenVec(DepenTupleTy LB, vector<DTNodeTy> &P)
{
    DTNodeTy L = LB.L, B = LB.B, A = LB.A;
    NodeID TF = LB.label;
    auto pi = std::find(P.begin(), P.end(), L); //find L
    if (pi == P.end())
        return;
    if (A != L) //if A!=L, the path dont contant L
        ++pi;
    CDGNode *nodeA = getCDGNode(getNodeIDFromBB(A->getBlock())); //all node fromL2B dependent on A
    CDGEdge::LabelType l = TF;
    CDSetElemTy tmpCDSetElem(nodeA->getId(), l); //依赖集中的元素
    for (; pi != P.end(); pi++)
    { //遍历路径P，添加依赖集
        NodeID cdNodeID = getNodeIDFromBB((*pi)->getBlock());
        assert(cdNodeID != -1);
        CDGNode *cdNode = getCDGNode(cdNodeID);
        addCDGEdge(nodeA, cdNode, l);
        auto iter = CDMap.find(cdNodeID);
        if (iter == CDMap.end())
        { //没找到就新建一个集合
            CDSetTy tmpCDSet;
            tmpCDSet.insert(tmpCDSetElem);
            CDMap.insert({cdNodeID, tmpCDSet});
        }
        else
        { //添加依赖元素，便是该节点依赖A
            (*iter).second.insert(tmpCDSetElem);
        }
    }
}

/*!
 * 找到ICFG节点的过程内的出边数量（不计算call和ret边）
 * @param iNode ICFG节点
 * @return
 */
u32_t CDG::icfgOutIntraEdgeNum(ICFGNode *iNode)
{
    u32_t n = 0;
    for (ICFGEdge *edge : iNode->getOutEdges())
        if (SVFUtil::dyn_cast<IntraCFGEdge>(edge))
            ++n;
    return n;
}

inline void ControlDependenceGraph::addCDGNode(CDGNode *node)
{
    _bb2CDGNodeID.insert(make_pair(node->getBasicBlock(), node->getId()));
    addGNode(node->getId(), node);
}

inline ControlCDGNode *ControlDependenceGraph::addControlCDGNode(const BasicBlock *nbb)
{
    ControlCDGNode *iNode = new ControlCDGNode(totalCDGNode++, nbb);
    addCDGNode(iNode);
    return iNode;
}

inline RegionCDGNode *ControlDependenceGraph::addRegionCDGNode()
{
    RegionCDGNode *iNode = new RegionCDGNode(totalCDGNode++);
    addCDGNode(iNode);
    return iNode;
}

inline void ControlDependenceGraph::addCDGEdge(CDGNode *s, CDGNode *d, CDGEdge::LabelType l)
{
    CDGEdge *edge = new CDGEdge(s, d, l);
    bool added1 = edge->getDstNode()->addIncomingEdge(edge);
    bool added2 = edge->getSrcNode()->addOutgoingEdge(edge);
}

inline void ControlDependenceGraph::removeCDGEdge(CDGEdge *edge)
{
    edge->getSrcNode()->removeOutgoingEdge(edge);
    edge->getDstNode()->removeIncomingEdge(edge);
    delete edge;
}

inline CDGNode *ControlDependenceGraph::getCDGNode(NodeID id)
{
    return getGNode(id);
}

bool ControlDependenceGraph::hasCDGNode(NodeID id)
{
    return hasGNode(id);
}

void ControlDependenceGraph::removeCDGNode(CDGNode *node)
{
    _bb2CDGNodeID.erase(node->getBasicBlock());
    removeGNode(node);
}

NodeID ControlDependenceGraph::getNodeIDFromBB(const BasicBlock *bb)
{
    auto iter = _bb2CDGNodeID.find(bb);
    if (iter != _bb2CDGNodeID.end())
        return iter->second;
    else
        return -1;
}

/**
 *  @brief 插入 RegionNoode 至 initial CDG 中，使其成为一个真正的 CDG
 */
void ControlDependenceGraph::addRegionNodeToCDG()
{
    // 1. 为每个存在控制依赖关系的结点，生成一个 Region Node。并使该结点依赖ReginNode，该ReginNode 依赖原先的CD结点。

    //      a. 首先单独获取图中的每一个节点（因为在遍历图时，不能修改图，否则会导致迭代器失效）
    list<CDGNode *> nodeWorklists;
    for (CDG::const_iterator nodeIt = this->begin(), nodeEit = this->end(); nodeIt != nodeEit; nodeIt++)
        nodeWorklists.push_back(nodeIt->second);

    //      b. 对每个存在入边的节点， 建立一个新的 RegionNode，并将其插入至图中，同时加入CDMap里
    for (auto iter = nodeWorklists.begin(); iter != nodeWorklists.end(); iter++)
    {
        /**
        @brief
            先将每一个带有入边的节点，其 dstNode <-- cde(T/F/None） -- srcNode
            转换成
                        +---------------------cde(T/F/None）--------------- +
                        V                                                   |
                    dstNode <-- none -- RegionNode                       srcNode
            最后再遍历所有入边，转换成
                    dstNode <-- none -- RegionNode <-- cde(T/F/None） -- srcNode
        @warning
            务必小心多个节点指向同一节点这种情况
        */

        CDGNode *dstNode = *iter;

        // 如果当前节点不存在入边，则直接跳过
        if (!(dstNode->hasIncomingEdge()))
            continue;

        // 新建 Region Node，并将其加入图中
        CDGNode *regionNode = addRegionCDGNode();

        // 遍历当前节点所有入边
        ///@warning 注意下列迭代过程会删除某些边导致迭代器失效
        for (CDGNode::const_iterator it = dstNode->InEdgeBegin(), eit = dstNode->InEdgeEnd(); it != eit;)
        {
            CDGEdge *edge = *(it++);
            CDGNode *srcNode = edge->getSrcNode();
            // 先加边 regionNode <-- edge -- srcNode 的边
            addCDGEdge(srcNode, regionNode, edge->getLabel());
            // 再删除 dstNode <-- edge -- srcNode 的边, 将 edge 从原先的图中删除
            removeCDGEdge(edge);
        }
        // 建立 node <-- none -- RegionNode
        CDGEdge *region2dst = new CDGEdge(regionNode, dstNode, CDGEdge::LabelType_None);
        regionNode->addOutgoingEdge(region2dst);
        dstNode->addIncomingEdge(region2dst);
    }
    dump("cdg_after_initR");
    /* 3. 后序遍历 后向支配树，计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT。
          判断如果R的控制依赖关系的前驱结点集合，是某个其他节点的控制依赖关系的前驱结点集合的子集，
          则使该节点直接依赖R来进行控制，以代替CD中的节点
    */
    /// @note geRoot 对于 postDT 返回nullptr, iff PDT 有多个 root 节点
    BasicBlock *root = PDT->getRoot();
    assert(root);
    auto pdtRootNode = PDT->getNode(root);
    PostOrderTraversalPDTNode(pdtRootNode);
    dump("cdg_after_mergeR");
    /**
     *  4. 对于具有多个控制依赖后继结点，并且具有相同关联标签L的任何谓词节点P，创建一个区域节点R。
     *     使图中具有控制依赖前驱结点 P 且标签L的每个节点都具有该区域节点 R。
     *     最后，使R成为具有相同标签的P的单个控制依赖关系的后继结点。
     */
    // 遍历所有的节点
    for (auto iter = nodeWorklists.begin(); iter != nodeWorklists.end(); iter++)
    {
        // 获取当前节点出边的labelType
        CDGNode *node = *iter;
        /// @note 添加对 switch 的处理
        map<CDGEdge::LabelType, list<CDGEdge *>> edgeMap;
        for (auto edgeIter = node->OutEdgeBegin(); edgeIter != node->OutEdgeEnd(); edgeIter++)
        {
            CDGEdge *edge = *edgeIter;
            /// note 这里无需考虑 edgeMap 中不存在的键值对，因为 operator[] 会自动创建
            edgeMap[edge->getLabel()].push_back(edge);
        }
        // 开始处理 edge->getLabel()
        for (auto mapIter = edgeMap.begin(); mapIter != edgeMap.end(); mapIter++)
        {
            CDGEdge::LabelType lableTy = mapIter->first;
            list<CDGEdge *> &edges = mapIter->second;
            // 如果某个 lableTy 只有一条边，或者是 None 类型的，则不进行处理
            if (edges.size() <= 1 || lableTy == CDGEdge::LabelType_None)
                continue;
            // 如果某个 lableTy 有多条出边，则新建一个 RegionNode，删除原先的边，并建立新边
            //      1. 新建 Region Node，并将其加入图中
            CDGNode *regionNode = addRegionCDGNode();
            this->addGNode(regionNode->getId(), regionNode);

            // 建立 RegionNode <-- lableTy -- node
            addCDGEdge(node, regionNode, lableTy);

            //      2. 遍历某一类型的所有边，将其删除并建立新边
            /// @note delete后指针失效但不影响list的顺序
            for (auto edgeIter = edges.begin(); edgeIter != edges.end();)
            {
                CDGEdge *edge = *(edgeIter++);
                CDGNode *anotherRegionNode = edge->getDstNode();
                // 先加边 anotherRegionNode <-- none -- regionNode 的边,标签应为空而不是(CDGEdge::LabelType)edge->getLabel()
                addCDGEdge(regionNode, anotherRegionNode, CDGEdge::LabelType_None);
                // 再删除 anotherRegionNode <-- edge -- node 的边, 将 edge 从原先的图中删除
                removeCDGEdge(edge);
            }
        }
    }
}

/**
 *  @brief 后序遍历后向支配树，并对遍历到的节点进行操作
 *  @param dtn 当前遍历到的后向支配树节点
 */
void ControlDependenceGraph::PostOrderTraversalPDTNode(const DomTreeNode *dtn)
{
    // 先遍历处理子节点
    for (auto iter = dtn->begin(); iter != dtn->end(); iter++)
        PostOrderTraversalPDTNode(*iter);
    // 最后处理根节点
    // 获取当前节点的 CDSet
    NodeID nodeID = this->getNodeIDFromBB(dtn->getBlock());
    CDSetTy cd = CDMap[nodeID];
    ///@warning 不是所有的PDTNode（或者说BasicBlock）都有对应的CDGNode
    CDGNode *currNode = this->getCDGNode(nodeID);
    CDGNode::GEdgeSetTy currInEdgeSet = currNode->getInEdges();
    // 一个简单的 check， 按理来说此时 所有的 control node 只会有一条入边，这条入边从 RegionNode 中连入
    if (currInEdgeSet.size() == 0)
        return;
    assert(currInEdgeSet.size() == 1);
    CDGNode *currRegionNode = (*(currInEdgeSet.begin()))->getSrcNode();

    // 开始遍历后向支配树中的所有直接子节点
    for (auto iter = dtn->begin(); iter != dtn->end(); iter++)
    {
        // 获取当前遍历的basicblock所对应的 CDSet
        NodeID childNodeID = this->getNodeIDFromBB(
            (*iter)->getBlock());
        CDSetTy childCDS = CDMap[childNodeID];
        //  a. 计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT
        CDSetTy INT = cd & childCDS;
        // 如果当前节点与子节点完全没有交集，则跳过当前子节点
        if (INT.size() == 0)
            continue;

        // 获取 child Region Node
        CDGNode *childNode = this->getCDGNode(childNodeID);
        CDGNode::GEdgeSetTy childInEdgeSet = childNode->getInEdges();
        assert(childInEdgeSet.size() == 1);
        CDGNode *childRegionNode = (*(childInEdgeSet.begin()))->getSrcNode();

        //  b. 如果 INT 与 CD 相等, 即 ParentCD \in ChildCD，则将子节点中对应的控制依赖用 R 来代替。
        if (cd == INT)
        {
            // 获取交集 INT 中对应 CD 的 edge
            // 遍历当前节点的入边，查找满足 INT 的对应 src 节点以及 edge label
            // 由于交集可能有1个以上的节点，因此为了保守起见，循环遍历删除child region node对应的 edge
            ///@warning 注意迭代器被删除后失效
            for (auto edgeIter = childRegionNode->InEdgeBegin(); edgeIter != childRegionNode->InEdgeEnd();)
            {
                CDGEdge *inEdge = *(edgeIter++);
                // 构造一个依赖对象
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getLabel());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if (INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 childRegionNode 中删除该边
                    removeCDGEdge(inEdge);
                }
            }
            // 最后从当前 currRegionNode 那边连过来一条线到 childRegionNode
            addCDGEdge(currRegionNode, childRegionNode, CDGEdge::LabelType_None);
        }

        //  c. 如果子节点中所有的控制依赖项 与 INT 相等，即 ChildCD \in ParentCD,
        //     则删除 R 中对应的控制依赖，并用 子节点的控制依赖来代替。
        else if (childCDS == INT)
        {
            // 获取交集 INT 中对应 CD 的 edge
            // 遍历当前节点的入边，查找满足 INT 的对应 src 节点以及 edge label
            // 由于交集可能有1个以上的节点，因此为了保守起见，循环遍历删除 curr region node对应的 edge
            ///@warning 注意迭代器失效
            for (auto edgeIter = currRegionNode->InEdgeBegin(); edgeIter != currRegionNode->InEdgeEnd();)
            {
                CDGEdge *inEdge = *(edgeIter++);
                // 构造一个依赖对象
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getLabel());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if (INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 currNode 中删除该边
                    removeCDGEdge(inEdge);
                }
            }
            // 最后从当前 childRegionNode 那边连过来一条线到 currRegionNode
            addCDGEdge(childRegionNode, currRegionNode, CDGEdge::LabelType_None);
        }
    }
}

void CDG::dump(const std::string &file)
{
    GraphPrinter::WriteGraphToFile(outs(), file, this);
};

void CDG::showSetS(DepenSSetTy &S, llvm::raw_ostream &O)
{
    for (auto it = S.begin(), ie = S.end(); it != ie; ++it)
    {
        assert(it->A->getBlock() != nullptr);
        assert(it->B->getBlock() != nullptr);
        O << "setS_A:"
          << " ";
        it->A->getBlock()->printAsOperand(O, false);
        O << ",\tB:"
          << " " << it->B->getBlock()->getName();
        O << ",\tL:"
          << " ";
        if (it->L->getBlock())
            it->L->getBlock()->printAsOperand(O, false);
        else
            O << " <<exit node>>";
        O << ",\tlabel: " << it->label << "\n";
    }
}

void CDG::showCDMap(CDMapTy CD, llvm::raw_ostream &O)
{
    for (auto it = CD.begin(), ie = CD.end(); it != ie; ++it)
    {
        O << "*** CD_A ****: ";
        getCDGNode((*it).first)->getBasicBlock()->printAsOperand(O, false);
        O << "\t\t elem: ";
        CDSetTy cdSet = (*it).second;
        for (auto sit = cdSet.begin(), sie = cdSet.end(); sit != sie; ++sit)
        {
            getCDGNode((*sit).getNodeID())->getBasicBlock()->printAsOperand(O, false);
            O << "\t ";
        }
        O << "\n";
    }
}

ControlDependenceEdge::ControlDependenceEdge(
    ControlDependenceNode *s,
    ControlDependenceNode *d,
    LabelType k)
    : GenericCDEdgeTy(s, d, k), label(k) {}

ControlDependenceNode::ControlDependenceNode(NodeID i, NodeType ty)
    : GenericCDNodeTy(i, ty), fun(NULL),_bb(NULL){};

ControlDependenceNode::~ControlDependenceNode()
{
    for(ControlDependenceEdge* edge : getOutEdges())
        delete edge;
}
FunEntryCDGNode::FunEntryCDGNode(NodeID id, const SVFFunction* f,const BasicBlock* bb) : ControlDependenceNode(id, FunEntryNode)
{
    fun = f;
    // if function is implemented
    _bb=bb;
}
void ControlDependenceNode::setBasicBlock(const BasicBlock *bb)
{
    _bb = bb;
}

const BasicBlock *ControlDependenceNode::getBasicBlock()
{
    return _bb;
}

ControlCDGNode::ControlCDGNode(NodeID id, const BasicBlock *bb)
    : CDGNode(id, CDGNode::NodeType::ControlNode)
{
    setBasicBlock(bb);
};

RegionCDGNode::RegionCDGNode(NodeID id)
    : CDGNode(id, CDGNode::NodeType::RegionNode)
{
    setBasicBlock(nullptr);
};

const std::string CDGNode::toString() const
{
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "CDGNode ID: " << getId();
    return rawstr.str();
}

const std::string ControlCDGNode::toString() const
{
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "ControlCDGNode ID: " << getId();
    return rawstr.str();
}

const std::string RegionCDGNode::toString() const
{
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "RegionCDGNode ID: " << getId();
    //    rawstr << " " << *getInst() << " {fun: " << getFun()->getName() << "}";
    return rawstr.str();
}
const std::string FunEntryCDGNode::toString() const
{
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "FunEntryCDGNode ID: " << getId();
    //    rawstr << " " << *getInst() << " {fun: " << getFun()->getName() << "}";
    return rawstr.str();
}
/*!
 * GraphTraits specialization
 */
namespace llvm
{
/*!
 * Write control dependence graph into dot file for debugging
 */
template <>
struct DOTGraphTraits<CDG *> : public DefaultDOTGraphTraits
{

    typedef CDGNode NodeType;
    typedef NodeType::iterator ChildIteratorType;

    DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple)
    {
    }

    /// Return name of the graph
    static std::string getGraphName(CDG *graph)
    {
        return "CDG";
    }

    /// Return label of a CDG node with two display mode
    /// Either you can choose to display the name of the value or the whole instruction
    static std::string getNodeLabel(CDGNode *node, CDG *)
    {
        std::string str;
        raw_string_ostream rawstr(str);
        // print bb info
        if (ControlCDGNode *cNode = SVFUtil::dyn_cast<ControlCDGNode>(node))
        {
            rawstr << "C: " << node->getId() << " \n";
        }
        else if (RegionCDGNode *cNode = SVFUtil::dyn_cast<RegionCDGNode>(node))
        {
            rawstr << "R: " << node->getId();
        }
        else
        {
        }
        if (node->getBasicBlock())
        {
            rawstr << "[";
            node->getBasicBlock()->printAsOperand(rawstr, false);
            rawstr << "]\n";
        }
        return rawstr.str();
    }

    static std::string getNodeAttributes(CDGNode *node, CDG *)
    {
        if (SVFUtil::isa<ControlCDGNode>(node))
        {
            //            return "shape=hexagon";
            //            return "shape=diamond";
            //            return "shape=circle";
            return "color=blue";
        }
        else if (SVFUtil::isa<RegionCDGNode>(node))
        {
            //            return "shape=doubleoctagon";
            //            return "shape=septagon";
            //            return "shape=Mcircle";
            //            return "shape=doublecircle";
            return "shape=circle";
        }
        else
        {
        }
        return "";
    }

    //
    template <class EdgeIter>
    static std::string getEdgeAttributes(CDGNode *, EdgeIter EI, CDG *)
    {
        const CDGEdge *edge = *(EI.getCurrent());
        assert(edge && "No edge found!!");
        if (edge->getLabel() == 0)
        {
            return "collor=blue";
        }
        return "";
    }

    //
    template <class EdgeIter>
    static std::string getEdgeSourceLabel(CDGNode *, EdgeIter EI)
    {
        CDGEdge *edge = *(EI.getCurrent());
        assert(edge && "No edge found!!");

        std::string str;
        raw_string_ostream rawstr(str);
        if (edge->getLabel() == CDGEdge::LabelType_None)
        {
            rawstr << "";
        }
        else if (edge->getSrcNode()->getOutEdges().size() == 2)
        {
            if (edge->getLabel() == 0)
                rawstr << "T";
            else if (edge->getLabel() == 1)
                rawstr << "F";
        }
        else if (edge->getLabel() == CDGEdge::LabelType_Default)
        {
            rawstr << "default";
        }
        else
        {
            rawstr << edge->getLabel();
        }
        return rawstr.str();
    }
};
} // End namespace llvm