#ifndef CDG_H
#define CDG_H

#include <map>

#include "Graphs/GenericGraph.h"
#include "Graphs/SVFG.h"


using namespace SVF;
using namespace llvm;
using namespace std;

class ControlDependenceGraph;
class ControlDependenceNode;
class ControlDependenceEdge;

typedef ControlDependenceGraph CDG;
typedef ControlDependenceNode CDGNode;
typedef ControlDependenceEdge CDGEdge;

// 设置ControlDependenceGraph 的类型
class ControlDependenceGraph : public GenericGraph<ControlDependenceNode,ControlDependenceEdge>
{
public:
    ControlDependenceGraph();
    void initCDG(SVFFunction *fun );//construct initial CDG
    inline void addCDGNode(NodeID id, NodeType* node);
    inline CDGNode* getCDGNode(NodeID id);
    inline bool hasCDGNode(NodeID id);
    inline void removeCDGNode(NodeType* node);
    NodeID getNodeIDFromBB(BasicBlock* bb);
private:
    // BasicBlock* -> CDG Node ID
    map<BasicBlock*, NodeID> _bb2CDGNodeID;
    PostDominatorTree* PDT = nullptr;
};


typedef GenericEdge<ControlDependenceNode> GenericCDEdgeTy;
class ControlDependenceEdge : public GenericCDEdgeTy
{
public:
    enum LabelType { T, F, None };
    ControlDependenceEdge(ControlDependenceNode* s, 
                       ControlDependenceNode* d, 
                       LabelType k);
};


typedef GenericNode<ControlDependenceNode, ControlDependenceEdge> GenericCDNodeTy;
class ControlDependenceNode : public GenericCDNodeTy
{
public:
    enum NodeType { ControlNode, RegionNode };
    ControlDependenceNode(NodeType ty);
    void setBasicBlock(BasicBlock* bb);
    BasicBlock* getBasicBlock();

private:
    BasicBlock* _bb;
    static NodeID _nextNodeID;
};

#endif