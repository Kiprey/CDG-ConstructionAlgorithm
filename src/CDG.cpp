#include "CDG.h"

ControlDependenceGraph::ControlDependenceGraph()
{
    /// @todo
}

void ControlDependenceGraph::initCDG(SVFFunction *fun )
{
    /// @todo
}

void ControlDependenceGraph::addCDGNode(NodeID id, CDGNode* node)
{
    _bb2CDGNodeID.insert(make_pair(node->getBasicBlock(), id));
    addGNode(id, node);
}
CDGNode* ControlDependenceGraph::getCDGNode(NodeID id)
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