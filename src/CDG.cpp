#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph(ICFG* icfg):icfg(icfg),totalCDGNode(0)
{
    PDT=new llvm::PostDominatorTree();
//    llvm::Function llvmFun(fun.getLLVMFun());
    /// @todo 构建PDT
}
/**
 * 构建后支配树
 */
void CDG::buildPDT(const SVFFunction *fun,DepenSSetTy &setS) {
    llvm::BasicBlock* entryBB=&(fun->getLLVMFun()->front());
    llvm::BasicBlock* exitBB=&(fun->getLLVMFun()->back());
    PDT->recalculate(*fun->getLLVMFun());
    BasicBlock *bb=llvm::BasicBlock ::Create(entryBB->getContext(), "before_entry", fun->getLLVMFun(),
                                             exitBB);

    DTNodeTy dnA=PDT->addNewBlock(bb,exitBB);
    DTNodeTy dnB=PDT->getNode(entryBB);
    DTNodeTy dnL=PDT->getNode(exitBB);
    PDT->print(outs());

    exitBB->getName();
    DepenTupleTy ABLT = {dnA,dnB,dnL, CDGEdge::LabelType::T};
    setS.insert(&ABLT);
    showSetS(setS,outs());
}

/*!
 * 构建初始的CDG图
 * @param fun
 */
void ControlDependenceGraph::initCDG(const SVFFunction *fun)
{
    DepenSSetTy setS;
    setS.clear();
    llvm::BasicBlock* entryBB=&(fun->getLLVMFun()->front());
    llvm::BasicBlock* exitBB=&(fun->getLLVMFun()->back());
    ///@queation 一旦封装到函数里，SetS就会莫名改变?
    PDT->recalculate(*fun->getLLVMFun());
    BasicBlock *bb=llvm::BasicBlock ::Create(entryBB->getContext(), "before_entry", fun->getLLVMFun(),
                                             exitBB);

    DTNodeTy dnA=PDT->addNewBlock(bb,exitBB);
    DTNodeTy dnB=PDT->getNode(entryBB);
    DTNodeTy dnL=PDT->getNode(exitBB);
    PDT->print(outs());

    DepenTupleTy ABLT = {dnA,dnB,dnL, CDGEdge::LabelType::T};
    setS.insert(&ABLT);
    //为每个PDT节点初始化一个CDG节点
    initCDGNodeFromPDT(PDT->getRootNode());
    showSetS(setS,outs());
    const BasicBlock *exbb = SVFUtil::getFunExitBB(fun->getLLVMFun());
    const BasicBlock *enbb = &(fun->getLLVMFun()->front());

    Set<const ICFGNode *> visited;
    findSSet(icfg->getFunEntryBlockNode(fun), visited, setS);//1.找到S集合
    showSetS(setS,outs());

    buildinitCDG(setS);//2.根据S集合计算支配关系，同时添加节点和边构建初始的CDG
    showCDMap(CDMap,outs());
    dump("wz_initial_cdg");

    addRegionNodeToCDG();//3.添加RegionNode
    dump("wz_final_cdg");
}
/*!
 * 遍历CFG的所有分支节点，找到S集合存入set'S中
 * @param iNode CFG节点
 * @param visited 记录已访问的CFG节点
 * @param setS S集合（A,B,L,TF）
 */
void CDG::findSSet(ICFGNode *iNode, Set<const ICFGNode *> &visited, DepenSSetTy &setS)
{
//    outs()<<"node_id::"<<iNode->getId()<<"node_outedge_num:"<<icfgOutIntraEdgeNum(iNode) <<"\n";//DEBUG
    if (visited.find(iNode) == visited.end())
        visited.insert(iNode);
    else
        return;
    if (SVFUtil::dyn_cast<FunExitBlockNode>(iNode)&& !SVFUtil::dyn_cast<IntraBlockNode>(iNode))
        return;
    if (icfgOutIntraEdgeNum(iNode) != 2)
    {
        for (ICFGEdge *edge : iNode->getOutEdges())
            findSSet(edge->getDstNode(), visited, setS);
        return;
    }
    //handle edges with condition to get S
    //traverse posdomtree to get P
    for (ICFGEdge* edge : iNode->getOutEdges())
    {
        IntraCFGEdge *intraEdge = SVFUtil::dyn_cast<IntraCFGEdge>(edge);
        if (intraEdge== nullptr)
            continue;
        findSSet(intraEdge->getDstNode(), visited, setS);
        NodeID TF = intraEdge->getBranchCondtion().second;
        const llvm::BasicBlock *bbA = intraEdge->getSrcNode()->getBB();
        const llvm::BasicBlock *bbB = intraEdge->getDstNode()->getBB();
        //判断该边的两个点是否存在支配关系，存在则存入S集合
        DTNodeTy dnA = PDT->getNode(bbA);
        DTNodeTy dnB = PDT->getNode(bbB);
        if (!PDT->properlyDominates(dnB, dnA))
        { //Return true iff B dominates A and A != B.
            DTNodeTy dnL = PDT->getNode(PDT->findNearestCommonDominator(bbA, bbB));
            DepenTupleTy ABLT = {dnA, dnB, dnL, TF};
            setS.insert(&ABLT);
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
 * 先根遍历后支配树，记录S集合中从L到B的路径
 * @param S S集合
 * @param P 记录路径
 */
void CDG::findPathL2B(const DepenSSetTy S, vector<DTNodeTy> &P)
{
    outs()<<"Traversing the DomTree From Root"<<"\n";//DEBUG
    findPathA2B(PDT->getRootNode(), S, P);
}

void CDG::findPathA2B(DTNodeTy A, DepenSSetTy S, vector<DTNodeTy> &P)
{
    outs()<<"Traver Dom_node_value:: "<<A;//DEBUG
//    A->getBlock()->printAsOperand(outs(),false);//DEBUG
    P.push_back(A);
    for (auto it = S.begin(), ie = S.end(); it != ie; it++)
    {
        if (A == (*it)->B)
        {
            outs()<<"Find B!!"<<"\n";//DEBUG
            handleDepenVec(*it, P);
            S.erase(it);
            break;
        }
    }
    for (auto it = A->begin(), ie = A->end(); it != ie; ++it)
        findPathA2B(*it, S, P);
    P.pop_back();
}


void CDG::initCDGNodeFromPDT(DTNodeTy dtNode) {
    addControlCDGNode(dtNode->getBlock());
    for(auto it=dtNode->begin(),ie=dtNode->end();it!=ie;++it)
        initCDGNodeFromPDT(*it);
}

/*!
 * 处理路径P生成CD集
 * @param LB
 * @param P
 */
void CDG::handleDepenVec(DepenTupleTy *LB, vector<DTNodeTy> &P)
{
    DTNodeTy L = LB->L, B = LB->B, A = LB->A;
    NodeID TF = LB->TF;
    auto pi = std::find(P.begin(), P.end(), L);        //find L
    if (pi == P.end())
        return;
    if (A!=L)                 //if A!=L, the path contant L,otherwise dont contain
        ++pi;
    CDGNode *nodeA = getCDGNode(getNodeIDFromBB(A->getBlock())); //all node below dependent on A
    CDGEdge::LabelType l = lable2bool(TF);
    CDSetElemTy tmpCDSetElem(nodeA->getId(), l);//依赖集中的元素
    for (; pi != P.end(); pi++)
    { //遍历路径P，添加节点和边
        NodeID cdNodeID=getNodeIDFromBB((*pi)->getBlock());
        assert(cdNodeID!=-1);
        CDGNode *cdNode=getCDGNode(cdNodeID);
        addCDGEdge(nodeA, cdNode, l);
        //得到依赖集（CD集）
        auto iter = CDMap.find(cdNodeID);
        if (iter == CDMap.end())
        { //没找到就就，新一个集合
            CDSetTy tmpCDSet;
            tmpCDSet.insert(tmpCDSetElem);
            CDMap.insert({cdNodeID, tmpCDSet});
        }
        else
        { //添加被依赖节点A到依赖的依赖集里
            (*iter).second.insert(tmpCDSetElem);
        }
    }
}

/*!
 * 将u32的lable类型转为CDGEdge::LableType类型
 * @param TF
 * @return
 */
CDGEdge::LabelType CDG::lable2bool(NodeID TF)
{
    switch (TF)
    {
    case 0:
        return CDGEdge::LabelType::F;
    case 1:
        return CDGEdge::LabelType::T;
    default:
        return CDGEdge::LabelType::None;
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
    for (ICFGEdge* edge : iNode->getOutEdges())
        if(SVFUtil::dyn_cast<IntraCFGEdge>(edge))
            ++n;
    return n;
}

inline void ControlDependenceGraph::addCDGNode(CDGNode *node)
{
    _bb2CDGNodeID.insert(make_pair(node->getBasicBlock(), node->getId()));
    addGNode(node->getId(), node);
}
inline ControlCDGNode *ControlDependenceGraph::addControlCDGNode(BasicBlock *nbb)
{
    ControlCDGNode *iNode = new ControlCDGNode(totalCDGNode++,nbb);
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

NodeID ControlDependenceGraph::getNodeIDFromBB(BasicBlock *bb)
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
            CDGEdge *src2region = new CDGEdge(srcNode, regionNode, (CDGEdge::LabelType)edge->getEdgeKind());
            srcNode->addOutgoingEdge(src2region);
            regionNode->addIncomingEdge(src2region);
            // 再删除 dstNode <-- edge -- srcNode 的边, 将 edge 从原先的图中删除
            srcNode->removeOutgoingEdge(edge);
            dstNode->removeIncomingEdge(edge);
            /// @warning 生命周期问题：图中的所有node & edge 该在什么时候进行释放，会不会产生生命周期问题
            delete edge;

            // 2. 在修改边指向 RegionNode 的过程中，可以顺便使用 Map 将 RegionNodeID映射到对应的控制依赖集。
            CDSetElemTy elem(srcNode->getId(), (CDGEdge::LabelType)src2region->getEdgeKind());
            /// @question 这里ControlNode的CD集已经有了，不需要重复添加，是否应该删除原本的CD集，将CD集中的项替换为{RNodeID,None},
            /// 再添加一个R节点的CD集
            // map 的 operator[] 在找不到 Set 时将自动创建，因此不必手动判断是否存在
            // 控制依赖集合映射： dstNodeID -> { { srcNodeID1, edgeLable1 }， { ... }， { ... } }
            //                 即 NodeID* -> CDSetTy
            CDMap[dstNode->getId()].insert(elem);
        }
        // 建立 node <-- none -- RegionNode
        CDGEdge *region2dst = new CDGEdge(regionNode, dstNode, CDGEdge::LabelType::None);
        regionNode->addOutgoingEdge(region2dst);
        dstNode->addIncomingEdge(region2dst);

    }

    /* 3. 后序遍历 后向支配树，计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT。
          判断如果R的控制依赖关系的前驱结点集合，是某个其他节点的控制依赖关系的前驱结点集合的子集，
          则使该节点直接依赖R来进行控制，以代替CD中的节点
    */
    /// @note geRoot 对于 postDT 返回nullptr, iff PDT 有多个 root 节点
    BasicBlock *root = PDT->getRoot();
    assert(root);
    auto pdtRootNode = PDT->getNode(root);
    PostOrderTraversalPDTNode(pdtRootNode);

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
        // 这里声明数组，方便代码编写
        list<CDGEdge *> edgeTF[2];
        for (auto edgeIter = node->OutEdgeBegin(); edgeIter != node->OutEdgeEnd(); edgeIter++)
        {
            CDGEdge *edge = *edgeIter;
            switch (edge->getEdgeKind())
            {
            case CDGEdge::LabelType::T:
                edgeTF[0].push_back(edge);
                break;
            case CDGEdge::LabelType::F:
                edgeTF[1].push_back(edge);
                break;
            }
        }
        // 开始处理 T F
        for (size_t i = 0; i < 2; i++)
        {
            list<CDGEdge *> &edges = edgeTF[i];
            // 如果 T/F 类型只有一条边，或者没有边，则不进行处理
            if (edges.size() <= 1)
                continue;
            // 如果 T/F 有多条出边，则新建一个 RegionNode，删除原先的边，并建立新边
            //      1. 新建 Region Node，并将其加入图中
            CDGNode *regionNode = addRegionCDGNode();
            this->addGNode(regionNode->getId(), regionNode);

            // 建立 RegionNode <-- T/F -- node
            CDGEdge *node2regionNode = new CDGEdge(node, regionNode,
                                                   (i == 0 ? CDGEdge::LabelType::T : CDGEdge::LabelType::F));
            node->addOutgoingEdge(node2regionNode);
            regionNode->addIncomingEdge(node2regionNode);

            //      2. 遍历某一类型的所有边，将其删除并建立新边
            for (auto edgeIter = edges.begin(); edgeIter != edges.end(); edgeIter++)
            {
                CDGEdge *edge = *edgeIter;
                CDGNode *anotherRegionNode = edge->getDstNode();
                // 先加边 anotherRegionNode <-- none -- regionNode 的边
                CDGEdge *region2anotherRegion = new CDGEdge(regionNode, anotherRegionNode, (CDGEdge::LabelType)edge->getEdgeKind());
                regionNode->addOutgoingEdge(region2anotherRegion);
                anotherRegionNode->addIncomingEdge(region2anotherRegion);
                // 再删除 anotherRegionNode <-- edge -- node 的边, 将 edge 从原先的图中删除
                node->removeOutgoingEdge(edge);
                anotherRegionNode->removeIncomingEdge(edge);
                delete edge;
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
    if(currInEdgeSet.size()==0)
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
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getEdgeKind());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if (INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 childRegionNode 中删除该边
                    inEdge->getSrcNode()->removeOutgoingEdge(inEdge);
                    inEdge->getDstNode()->removeIncomingEdge(inEdge);
                    delete inEdge;
                }
            }
            // 最后从当前 currRegionNode 那边连过来一条线到 childRegionNode
            CDGEdge *newEdge = new CDGEdge(currRegionNode, childRegionNode, CDGEdge::LabelType::None);
            currRegionNode->addOutgoingEdge(newEdge);
            childRegionNode->addIncomingEdge(newEdge);
        }

        //  c. 如果子节点中所有的控制依赖项 与 INT 相等，即 ChildCD \in ParentCD,
        //     则删除 R 中对应的控制依赖，并用 子节点的控制依赖来代替。
        else if (childCDS == INT)
        {
            // 获取交集 INT 中对应 CD 的 edge
            // 遍历当前节点的入边，查找满足 INT 的对应 src 节点以及 edge label
            // 由于交集可能有1个以上的节点，因此为了保守起见，循环遍历删除 curr region node对应的 edge
            for (auto edgeIter = currRegionNode->InEdgeBegin(); edgeIter != currRegionNode->InEdgeEnd(); edgeIter++)
            {
                CDGEdge *inEdge = *edgeIter;
                // 构造一个依赖对象
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getEdgeKind());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if (INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 currNode 中删除该边
                    inEdge->getSrcNode()->removeOutgoingEdge(inEdge);
                    inEdge->getDstNode()->removeIncomingEdge(inEdge);
                    delete inEdge;
                }
            }
            // 最后从当前 childRegionNode 那边连过来一条线到 currRegionNode
            CDGEdge *newEdge = new CDGEdge(childRegionNode, currRegionNode, CDGEdge::LabelType::None);
            childRegionNode->addOutgoingEdge(newEdge);
            currRegionNode->addIncomingEdge(newEdge);
        }
    }
}


void CDG::dump(const std::string& file){
    GraphPrinter::WriteGraphToFile(outs(), file, this);
};

void CDG::showSetS(DepenSSetTy &S,llvm::raw_ostream &O){
    for (auto it=S.begin(),ie=S.end();it!=ie;++it){
        O<<"setS_A:"<<" "<<(*it)->A->getBlock()->getName();
        O<<"\t,B:"<<" "<<(*it)->B->getBlock()->getName();
        O<<"\t,L:"<<" "<<(*it)->L->getBlock()->getName();
        O<<"\t,TF:"<<(*it)->TF<<"\n";
    }
}

void CDG::showCDMap(CDMapTy CD,llvm::raw_ostream &O){
    for (auto it=CD.begin(),ie=CD.end();it!=ie;++it){
        O<<"***CD_A****:";
        getCDGNode((*it).first)->getBasicBlock()->printAsOperand(O,false);
        O<<"\t\t elem:";
        CDSetTy cdSet=(*it).second;
        for(auto sit=cdSet.begin(),sie=cdSet.end();sit!=sie;++sit){
            getCDGNode((*sit).getNodeID())->getBasicBlock()->printAsOperand(O,false);
            O<<"\t ";
        }
        O<<"\n";
    }
}

ControlDependenceEdge::ControlDependenceEdge(
    ControlDependenceNode *s,
    ControlDependenceNode *d,
    LabelType k)
    : GenericCDEdgeTy(s, d, k) {}


ControlDependenceNode::ControlDependenceNode(NodeID i,NodeType ty)
    : GenericCDNodeTy(i, ty) , _bb(NULL){};

void ControlDependenceNode::setBasicBlock(BasicBlock *bb)
{
    _bb = bb;
}

BasicBlock *ControlDependenceNode::getBasicBlock()
{
    return _bb;
}
ControlCDGNode::ControlCDGNode(NodeID id,llvm::BasicBlock* bb)
        : CDGNode(id,CDGNode::NodeType::ControlNode) {
    setBasicBlock(bb);
};
RegionCDGNode::RegionCDGNode(NodeID id)
        : CDGNode(id,CDGNode::NodeType::RegionNode) {
    setBasicBlock(nullptr);
};



const std::string CDGNode::toString() const {
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "CDGNode ID: " << getId();
    return rawstr.str();
}


const std::string ControlCDGNode::toString() const {
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "ControlCDGNode ID: " << getId();
    return rawstr.str();
}


const std::string RegionCDGNode::toString() const {
    std::string str;
    raw_string_ostream rawstr(str);
    rawstr << "RegionCDGNode ID: " << getId();
//    rawstr << " " << *getInst() << " {fun: " << getFun()->getName() << "}";
    return rawstr.str();
}


/*!
 * GraphTraits specialization
 */
namespace llvm {
/*!
 * Write control dependence graph into dot file for debugging
 */
    template<>
struct DOTGraphTraits<CDG *> : public DefaultDOTGraphTraits {

    typedef CDGNode NodeType;
    typedef NodeType::iterator ChildIteratorType;
    DOTGraphTraits(bool isSimple = false) :
            DefaultDOTGraphTraits(isSimple)
    {
    }

    /// Return name of the graph
    static std::string getGraphName(CDG *graph)
    {
        return "CDG";
    }

    /// Return label of a CDG node with two display mode
    /// Either you can choose to display the name of the value or the whole instruction
    static std::string getNodeLabel(CDGNode *node, CDG*)
    {
        std::string str;
        raw_string_ostream rawstr(str);
        // print bb info
        if (ControlCDGNode* cNode = SVFUtil::dyn_cast<ControlCDGNode>(node))
        {
            rawstr << "C: "<< node->getId() << " \n";
        }
        else if(RegionCDGNode* cNode = SVFUtil::dyn_cast<RegionCDGNode>(node)){
            rawstr << "R: "<<node->getId();
        }
        else
        {
        }
        if (node->getBasicBlock()){
            rawstr << "[";
            node->getBasicBlock()->printAsOperand(rawstr,false);
            rawstr << "]\n";
        }
        return rawstr.str();
    }

    static std::string getNodeAttributes(CDGNode *node, CDG*)
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
//    template<class EdgeIter>
//    static std::string getEdgeAttributes(PAGNode*, EdgeIter EI, PAG*)
//    {
//        const PAGEdge* edge = *(EI.getCurrent());
//        assert(edge && "No edge found!!");
//        if (SVFUtil::isa<AddrPE>(edge))
//        {
//            return "color=green";
//        }
//        else
//        {
//            assert(0 && "No such kind edge!!");
//        }
//    }
//
//    template<class EdgeIter>
//    static std::string getEdgeSourceLabel(PAGNode*, EdgeIter EI)
//    {
//        const PAGEdge* edge = *(EI.getCurrent());
//        assert(edge && "No edge found!!");
//        if(const CallPE* calledge = SVFUtil::dyn_cast<CallPE>(edge))
//        {
//            const Instruction* callInst= calledge->getCallSite()->getCallSite();
//            return SVFUtil::getSourceLoc(callInst);
//        }
//        else if(const RetPE* retedge = SVFUtil::dyn_cast<RetPE>(edge))
//        {
//            const Instruction* callInst= retedge->getCallSite()->getCallSite();
//            return SVFUtil::getSourceLoc(callInst);
//        }
//        return "";
//    }
};
} // End namespace llvm