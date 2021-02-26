#include <list>

#include "CDG.h"
#include "CDmap.h"

void PostOrderTraversalPDTNode(CDMap cdMap, CDG* cdg, const DomTreeNode* pdn)
{
    // 先遍历处理子节点
    for(auto iter = pdn->begin(); iter != pdn->end(); iter++)
        PostOrderTraversalPDTNode(cdMap, cdg, *iter);

    // 最后处理根节点
    // 获取当前节点的 CDSet
    CDSet cd = cdMap[
        cdg->getNodeIDFromBB(
            pdn->getBlock())];
    for(auto iter = pdn->begin(); iter != pdn->end(); iter++)
    {
        // 获取当前遍历的basicblock所对应的 CDSet;
        CDSet childCDS = cdMap[
                            cdg->getNodeIDFromBB(
                                (*iter)->getBlock())];
        //  a. 计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT
        CDSet INT = cd & childCDS;
        //  b. 如果 INT 与 CD 相等，则将子节点中对应的控制依赖用 R 来代替。
        if(cd == INT)
        {

        }
        else
        //  c. 如果存在子节点的控制依赖都是 INT，则删除 R 中对应的控制依赖，并用对应 子节点的控制依赖来代替。
        {
            //  此处 childCDS 必须是 cd 中的子集，即与交集 INT 相等
            assert(childCDS == INT);
            
        }
    }
}

// 插入 RegionNoode 至 initial CDG 中，使其成为一个真正的 CDG
void addRegionNodeToCDG(CDG* cdg, const PostDominatorTree* PDT)
{
    CDMap cdMap;
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
        // 如果一个 node 没有入边，也就是不依赖于任何节点，则不用添加RegionNode
        if(! dstNode->hasIncomingEdge())
            continue;

        // 新建 Region Node，并将其加入图中
        CDGNode* regionNode = new CDGNode(CDGNode::NodeType::RegionNode);
        cdg->addGNode(regionNode->getId(), regionNode);
        
        // 建立 node <-- none -- RegionNode
        CDGEdge* region2dst = new CDGEdge(regionNode, dstNode, CDGEdge::LabelType::None); 
        dstNode->addIncomingEdge(region2dst);
        regionNode->addOutgoingEdge(region2dst);

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
            /// @todo 生命周期问题：图中的所有node & edge 该在什么时候进行释放，会不会产生生命周期问题
            delete edge;

    // 2. 在修改边指向 RegionNode 的过程中，可以顺便使用 Map 将 RegionNodeID映射到对应的控制依赖集。
            CDSetElem elem(srcNode->getId(), (CDGEdge::LabelType)src2region->getEdgeKind());
            // map 的 operator[] 在找不到 Set 时将自动创建，因此不必手动判断是否存在
            // 控制依赖集合映射： (dstNode->getId()) -> { { srcNodeID1, edgeLable1 }， { ... }， { ... } }
            //                 即 NodeID* -> CDSet
            cdMap[dstNode->getId()].insert(elem);
        }
    }

    // 3. 后序遍历 后向支配树，计算当前结点 N 的 CD 与 后支配树中 N 的每个直接子结点的CD 的交集 INT。如果 INT 与 CD 相等，则将子节点中对应的控制依赖用 R 来代替。
    //    如果存在子节点的控制依赖都是 INT，则删除 R 中对应的控制依赖，并用对应 子节点的控制依赖来代替。
    /// @note geRoot 对于 postDT 返回nullptr, iff PDT 有多个 root 节点
    BasicBlock* root = PDT->getRoot();
    assert(root);
    auto pdtRootNode = PDT->getNode(root);
    PostOrderTraversalPDTNode(cdMap, cdg, pdtRootNode);
}