#include <list>

#include "CDG.h"

// 一个简单的声明
void PostOrderTraversalPDTNode(CDMapTy cdMap, CDG* cdg, const DomTreeNode* dtn);

/**
 *  @brief 插入 RegionNoode 至 initial CDG 中，使其成为一个真正的 CDG
 *  @param cdg 初始 CDG 图
 *  @param PDT 初始 CDG 图的所有节点所构成的后向支配树
 */
void addRegionNodeToCDG(CDG* cdg, const PostDominatorTree* PDT)
{
    CDMapTy cdMap;
    // 1. 为每个存在控制依赖关系的结点，生成一个 Region Node。并使该结点依赖ReginNode，该ReginNode 依赖原先的CD结点。

    //      a. 首先单独获取图中的每一个节点（因为在遍历图时，不能修改图，否则会导致迭代器失效）
    list<CDGNode*> nodeWorklists;
    for (CDG::const_iterator nodeIt = cdg->begin(), nodeEit = cdg->end(); nodeIt != nodeEit; nodeIt++)
        nodeWorklists.push_back(nodeIt->second);

    //      b. 对每个存在入边的节点， 建立一个新的 RegionNode，并将其插入至图中，同时加入CDMap里
    for(auto iter = nodeWorklists.begin(); iter != nodeWorklists.end(); iter++)
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

        CDGNode* dstNode = *iter;

        // 如果当前节点不存在入边，则直接跳过
        if(!(dstNode->hasIncomingEdge()))
            continue;

        // 新建 Region Node，并将其加入图中
        CDGNode* regionNode = new CDGNode(CDGNode::NodeType::RegionNode);
        cdg->addGNode(regionNode->getId(), regionNode);
        
        // 建立 node <-- none -- RegionNode
        CDGEdge* region2dst = new CDGEdge(regionNode, dstNode, CDGEdge::LabelType::None); 
        regionNode->addOutgoingEdge(region2dst);
        dstNode->addIncomingEdge(region2dst);

        // 遍历当前节点所有入边
        for (CDGNode::const_iterator it = dstNode->InEdgeBegin(), eit = dstNode->InEdgeEnd(); it != eit; ++it)
        {
            CDGEdge* edge = *it;
            CDGNode* srcNode = edge->getSrcNode();
            // 先加边 regionNode <-- edge -- srcNode 的边
            CDGEdge* src2region = new CDGEdge(srcNode, regionNode, (CDGEdge::LabelType)edge->getEdgeKind()); 
            srcNode->addOutgoingEdge(src2region);
            regionNode->addIncomingEdge(src2region);
            // 再删除 dstNode <-- edge -- srcNode 的边, 将 edge 从原先的图中删除
            srcNode->removeOutgoingEdge(edge);
            dstNode->removeIncomingEdge(edge);
            /// @warning 生命周期问题：图中的所有node & edge 该在什么时候进行释放，会不会产生生命周期问题
            delete edge;

    // 2. 在修改边指向 RegionNode 的过程中，可以顺便使用 Map 将 RegionNodeID映射到对应的控制依赖集。
            CDSetElemTy elem(srcNode->getId(), (CDGEdge::LabelType)src2region->getEdgeKind());
            // map 的 operator[] 在找不到 Set 时将自动创建，因此不必手动判断是否存在
            // 控制依赖集合映射： dstNodeID -> { { srcNodeID1, edgeLable1 }， { ... }， { ... } }
            //                 即 NodeID* -> CDSetTy
            cdMap[dstNode->getId()].insert(elem);
        }
    }

    /* 3. 后序遍历 后向支配树，计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT。
          判断如果R的控制依赖关系的前驱结点集合，是某个其他节点的控制依赖关系的前驱结点集合的子集，
          则使该节点直接依赖R来进行控制，以代替CD中的节点
    */
    /// @note geRoot 对于 postDT 返回nullptr, iff PDT 有多个 root 节点
    BasicBlock* root = PDT->getRoot();
    assert(root);
    auto pdtRootNode = PDT->getNode(root);
    PostOrderTraversalPDTNode(cdMap, cdg, pdtRootNode);
    
    /**
     *  4. 对于具有多个控制依赖后继结点，并且具有相同关联标签L的任何谓词节点P，创建一个区域节点R。
     *     使图中具有控制依赖前驱结点 P 且标签L的每个节点都具有该区域节点 R。 
     *     最后，使R成为具有相同标签的P的单个控制依赖关系的后继结点。 
     */
    // 遍历所有的节点
    for(auto iter = nodeWorklists.begin(); iter != nodeWorklists.end(); iter++)
    {
        // 获取当前节点出边的labelType
        CDGNode* node = *iter;
        // 这里声明数组，方便代码编写
        list<CDGEdge*> edgeTF[2];
        for(auto edgeIter = node->OutEdgeBegin(); edgeIter != node->OutEdgeEnd(); edgeIter++)
        {
            CDGEdge* edge = *edgeIter;
            switch(edge->getEdgeKind())
            {
            case CDGEdge::LabelType::T: edgeTF[0].push_back(edge); break;
            case CDGEdge::LabelType::F: edgeTF[1].push_back(edge); break;
            }
        }
        // 开始处理 T F
        for(size_t i = 0; i < 2; i++)
        {
            list<CDGEdge*>& edges = edgeTF[i];
            // 如果 T/F 类型只有一条边，或者没有边，则不进行处理
            if(edges.size() <= 1)
                continue;
            // 如果 T/F 有多条出边，则新建一个 RegionNode，删除原先的边，并建立新边
            //      1. 新建 Region Node，并将其加入图中
            CDGNode* regionNode = new CDGNode(CDGNode::NodeType::RegionNode);
            cdg->addGNode(regionNode->getId(), regionNode);
            
            // 建立 RegionNode <-- T/F -- node
            CDGEdge* node2regionNode = new CDGEdge(node, regionNode, 
                (i == 0 ? CDGEdge::LabelType::T : CDGEdge::LabelType::F)); 
            node->addOutgoingEdge(node2regionNode);
            regionNode->addIncomingEdge(node2regionNode);
            
            //      2. 遍历某一类型的所有边，将其删除并建立新边
            for(auto edgeIter = edges.begin(); edgeIter!= edges.end(); edgeIter++)
            {
                CDGEdge* edge = *edgeIter;
                CDGNode* anotherRegionNode = edge->getDstNode();
                // 先加边 anotherRegionNode <-- none -- regionNode 的边
                CDGEdge* region2anotherRegion = new CDGEdge(regionNode, anotherRegionNode, (CDGEdge::LabelType)edge->getEdgeKind()); 
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
 *  @param cdMap 存放着控制依赖关系的 map 集合
 *  @param cdg 简单加上 Region Node 后的 CDG 图
 *  @param dtn 当前遍历到的后向支配树节点
 */
void PostOrderTraversalPDTNode(CDMapTy cdMap, CDG* cdg, const DomTreeNode* dtn)
{
    // 先遍历处理子节点
    for(auto iter = dtn->begin(); iter != dtn->end(); iter++)
        PostOrderTraversalPDTNode(cdMap, cdg, *iter);

    // 最后处理根节点
    // 获取当前节点的 CDSet
    NodeID nodeID = cdg->getNodeIDFromBB(   dtn->getBlock() );
    CDSetTy cd = cdMap[nodeID];

    CDGNode* currNode = cdg->getCDGNode(nodeID);
    CDGNode::GEdgeSetTy currInEdgeSet = currNode->getInEdges();
    // 一个简单的 check， 按理来说此时 所有的 control node 只会有一条入边，这条入边从 RegionNode 中连入
    assert(currInEdgeSet.size() == 1);
    CDGNode* currRegionNode = (*(currInEdgeSet.begin()))->getSrcNode();

    // 开始遍历后向支配树中的所有直接子节点
    for(auto iter = dtn->begin(); iter != dtn->end(); iter++)
    {
        // 获取当前遍历的basicblock所对应的 CDSet
        NodeID childNodeID = cdg->getNodeIDFromBB(
                                (*iter)->getBlock());
        CDSetTy childCDS = cdMap[childNodeID];
        //  a. 计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT
        CDSetTy INT = cd & childCDS;
        // 如果当前节点与子节点完全没有交集，则跳过当前子节点
        if(INT.size() == 0)
            continue;

        // 获取 child Region Node
        CDGNode* childNode = cdg->getCDGNode(childNodeID);
        CDGNode::GEdgeSetTy childInEdgeSet = childNode->getInEdges();
        assert(childInEdgeSet.size() == 1);
        CDGNode* childRegionNode = (*(childInEdgeSet.begin()))->getSrcNode();
            
        //  b. 如果 INT 与 CD 相等, 即 ParentCD \in ChildCD，则将子节点中对应的控制依赖用 R 来代替。
        if(cd == INT)
        {
            // 获取交集 INT 中对应 CD 的 edge
            // 遍历当前节点的入边，查找满足 INT 的对应 src 节点以及 edge label
            // 由于交集可能有1个以上的节点，因此为了保守起见，循环遍历删除child region node对应的 edge
            for(auto edgeIter = childRegionNode->InEdgeBegin(); edgeIter != childRegionNode->InEdgeEnd(); edgeIter++)
            {
                CDGEdge * inEdge = *edgeIter;
                // 构造一个依赖对象
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getEdgeKind());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if(INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 childRegionNode 中删除该边
                    inEdge->getSrcNode()->removeOutgoingEdge(inEdge);
                    inEdge->getDstNode()->removeIncomingEdge(inEdge);
                    delete inEdge;
                }
            }
            // 最后从当前 currRegionNode 那边连过来一条线到 childRegionNode
            CDGEdge* newEdge = new CDGEdge(currRegionNode, childRegionNode, CDGEdge::LabelType::None); 
            currRegionNode->addOutgoingEdge(newEdge);
            childRegionNode->addIncomingEdge(newEdge);
        }

        //  c. 如果子节点中所有的控制依赖项 与 INT 相等，即 ChildCD \in ParentCD, 
        //     则删除 R 中对应的控制依赖，并用 子节点的控制依赖来代替。
        else if(childCDS == INT)
        {
            // 获取交集 INT 中对应 CD 的 edge
            // 遍历当前节点的入边，查找满足 INT 的对应 src 节点以及 edge label
            // 由于交集可能有1个以上的节点，因此为了保守起见，循环遍历删除 curr region node对应的 edge
            for(auto edgeIter = currRegionNode->InEdgeBegin(); edgeIter != currRegionNode->InEdgeEnd(); edgeIter++)
            {
                CDGEdge * inEdge = *edgeIter;
                // 构造一个依赖对象
                CDSetElemTy cdElem(inEdge->getSrcID(), (CDGEdge::LabelType)inEdge->getEdgeKind());
                // 去 INT 中查找是否存在当前依赖对象，如果存在：
                if(INT.find(cdElem) != INT.end())
                {
                    // 从 srcNode 和 currNode 中删除该边
                    inEdge->getSrcNode()->removeOutgoingEdge(inEdge);
                    inEdge->getDstNode()->removeIncomingEdge(inEdge);
                    delete inEdge;
                }
            }
            // 最后从当前 childRegionNode 那边连过来一条线到 currRegionNode
            CDGEdge* newEdge = new CDGEdge(childRegionNode, currRegionNode, CDGEdge::LabelType::None); 
            childRegionNode->addOutgoingEdge(newEdge);
            currRegionNode->addIncomingEdge(newEdge);
        }
    }
}