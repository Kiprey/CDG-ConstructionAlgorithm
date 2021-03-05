#ifndef CDG_H
#define CDG_H

#include <map>
#include <set>
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
    NodeID getNodeID() const;
    CDGEdge::LabelType getLabel();

private:
    NodeID _CDG_ID;
    CDGEdge::LabelType _label;
};

class ControlDependenceGraph : public GenericGraph<ControlDependenceNode, ControlDependenceEdge>
{
public:

    typedef Map<NodeID, CDGNode *> CDGNodeIDToNodeMapTy;
    typedef CDGEdge::CDGEdgeSetTy CDGEdgeSetTy;
    typedef CDGNodeIDToNodeMapTy::iterator iterator;
    typedef CDGNodeIDToNodeMapTy::const_iterator const_iterator;

    typedef DomTreeNodeBase<llvm::BasicBlock> *DTNodeTy;
    typedef struct DepenTuple{
        DTNodeTy A;
        DTNodeTy B;
        DTNodeTy L;
        NodeID TF;
        friend bool operator<(const  DepenTuple&n1, const DepenTuple &n2) {
            return n1.A->getLevel() >= n2.A->getLevel();
        }
    }DepenTupleTy;
    typedef set<DepenTupleTy> DepenSSetTy;
    typedef vector<DTNodeTy> DepenVecTy;

public:
    ControlDependenceGraph(ICFG* icfg);

    void buildPDT(const SVFFunction *fun,DepenSSetTy &setS);
    void initCDG(const SVFFunction *fun); //construct initial CDG
    void initCDGNodeFromPDT(DTNodeTy dtNode);//初始化一个节点
    void findSSet(ICFGNode *entryNode, Set<const ICFGNode *> &visited, DepenSSetTy &setS);
    void buildinitCDG(DepenSSetTy S);
    u32_t icfgOutIntraEdgeNum(ICFGNode *iNode);
    void findPathL2B(DepenSSetTy &S, vector<DTNodeTy> &P);
    void findPathA2B(DTNodeTy A, DepenSSetTy &S, vector<DTNodeTy> &P);
    void handleDepenVec(DepenTupleTy LB, vector<DTNodeTy> &P);
    CDGEdge::LabelType lable2bool(NodeID TF);

    void addCDGEdge(CDGNode *s, CDGNode *d, CDGEdge::LabelType l);

    inline CDGNode *getCDGNode(NodeID id);
    inline bool hasCDGNode(NodeID id);
    inline void removeCDGNode(NodeType *node);
    NodeID getNodeIDFromBB(BasicBlock *bb);

    void showSetS(DepenSSetTy &S,llvm::raw_ostream &O);
    void showCDMap(CDMapTy CD,llvm::raw_ostream &O);
    /// Dump graph into dot file
    void dump(const std::string& file);

    NodeID totalCDGNode;
private:
    map<BasicBlock *, NodeID> _bb2CDGNodeID;
    PostDominatorTree *PDT ;
    CDMapTy CDMap;
    ICFG* icfg;
    inline void addCDGNode(NodeType *node);
    inline ControlCDGNode *addControlCDGNode(BasicBlock *nbb);
    inline RegionCDGNode *addRegionCDGNode();
    void PostOrderTraversalPDTNode(const DomTreeNode *dtn);
    void addRegionNodeToCDG();
};

namespace llvm
{
/* !
 * GraphTraits specializations for generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */
    template<> struct GraphTraits<CDGNode*> : public GraphTraits<SVF::GenericNode<CDGNode,CDGEdge>*  >
    {
    };

/// Inverse GraphTraits specializations for call graph node, it is used for inverse traversal.
    template<>
    struct GraphTraits<Inverse<CDGNode *> > : public GraphTraits<Inverse<SVF::GenericNode<CDGNode,CDGEdge>* > >
    {
    };

    template<> struct GraphTraits<CDG*> : public GraphTraits<SVF::GenericGraph<CDGNode,CDGEdge>* >
    {
        typedef CDGNode *NodeRef;
    };

} // End namespace llvm

#endif
