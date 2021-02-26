#ifndef CDG_H
#define CDG_H

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
private:
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
    static size_t _nextNodeID;
};

#endif