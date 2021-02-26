#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph()
{
    /// @todo
}

void ControlDependenceGraph::initCDG(SVFFunction *fun )
{
    /// @todo
}

ControlDependenceEdge::ControlDependenceEdge(
        ControlDependenceNode* s, 
        ControlDependenceNode* d, 
        LabelType k)  
    : GenericCDEdgeTy(s,d,k) {}

size_t ControlDependenceNode::_nextNodeID = 0;

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