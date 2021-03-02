#ifndef CDG_H
#define CDG_H

#include <map>

#include "Graphs/GenericGraph.h"
#include "Graphs/SVFG.h"
#include "CDGEdge.h"
#include "CDGNode.h"

using namespace SVF;
using namespace llvm;
using namespace std;

class ControlDependenceGraph;
class ControlDependenceSetElement;

typedef ControlDependenceGraph CDG;
// 设置数据结构 set
typedef set<ControlDependenceSetElement> ControlDependenceSet;
// 实现两个 ControlDependenceSet 取交集的函数
ControlDependenceSet operator&(const ControlDependenceSet &cds1, const ControlDependenceSet &cds2);
// 设置数据结构 map
typedef map<NodeID, ControlDependenceSet> ControlDependenceMap;
// 设置缩写
typedef ControlDependenceMap CDMapTy;
typedef ControlDependenceSetElement CDSetElemTy;
typedef ControlDependenceSet CDSetTy;
// 设置ControlDependenceGraph 的类型

// 某个节点依赖集所存放的单个元素。以 { nodeID, edgeLabel } 为一个整体存放
// 例如依赖 { 1, F }
class ControlDependenceSetElement
{
public:
    ControlDependenceSetElement(NodeID _id, CDGEdge::LabelType ty);
    // 排序时优先按照 节点编号 排序，例如 {1, T} < {1, F} < {2, T}
    bool operator<(const ControlDependenceSetElement &e) const;
    bool operator==(const ControlDependenceSetElement &e) const;
    NodeID getNodeID();
    CDGEdge::LabelType getLabel();

private:
    NodeID _CDG_ID;
    CDGEdge::LabelType _label;
};

class ControlDependenceGraph : public GenericGraph<ControlDependenceNode, ControlDependenceEdge>
{
public:
    typedef DomTreeNodeBase<llvm::BasicBlock> *DTNodeTy;
    typedef struct
    {
        DTNodeTy A;
        DTNodeTy B;
        DTNodeTy L;
        NodeID TF;
    } DepenTupleTy;
    typedef Set<DepenTupleTy *> DepenSSetTy;
    typedef vector<DTNodeTy> DepenVecTy;

public:
    ControlDependenceGraph();

    void initCDG(SVFFunction *fun); //construct initial CDG
    void findSSet(ICFGNode *entryNode, Set<const ICFGNode *> &visited, DepenSSetTy &setS);
    void buildinitCDG(DepenSSetTy S);
    u32_t icfgOutEdgeNum(ICFGNode *iNode);
    void findPathL2B(DepenSSetTy S, vector<DTNodeTy> &P);
    void findPathA2B(DTNodeTy A, DepenSSetTy S, vector<DTNodeTy> &P);
    void handleDepenVec(DepenTupleTy *LB, vector<DTNodeTy> &P);
    CDGEdge::LabelType lable2bool(NodeID TF);

    void addCDGEdge(CDGNode *s, CDGNode *d, CDGEdge::LabelType l);

    inline CDGNode *getCDGNode(NodeID id);
    inline bool hasCDGNode(NodeID id);
    inline void removeCDGNode(NodeType *node);
    NodeID getNodeIDFromBB(BasicBlock *bb);

private:
    // BasicBlock* -> CDG Node ID
    map<BasicBlock *, NodeID> _bb2CDGNodeID;
    PostDominatorTree *PDT = nullptr;
    CDMapTy CDMap;
    inline void addCDGNode(NodeType *node);
    inline CDGNode *addControlCDGNode(BasicBlock *nbb);
    inline CDGNode *addRegionCDGNode();
    void PostOrderTraversalPDTNode(const DomTreeNode *dtn);
    void addRegionNodeToCDG();
};

#endif
