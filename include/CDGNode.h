//
// Created by wz on 2021/3/2.
//

#ifndef CDG_CONSTRUCT_CDGNODE_H
#define CDG_CONSTRUCT_CDGNODE_H

#include "CDGEdge.h"
class ControlDependenceNode;
typedef ControlDependenceNode CDGNode;
typedef GenericNode<ControlDependenceNode, ControlDependenceEdge> GenericCDNodeTy;
class ControlDependenceNode : public GenericCDNodeTy
{
public:
    enum NodeType
    {
        ControlNode,
        RegionNode
    };
    ControlDependenceNode(NodeType ty);
    void setBasicBlock(BasicBlock *bb);
    BasicBlock *getBasicBlock();

private:
    BasicBlock *_bb;
    static NodeID _nextNodeID;
};

class ControlCDGNode : public CDGNode
{
public:
    ControlCDGNode();
    void setBasicBlock(BasicBlock *bb);
    BasicBlock *getBasicBlock();

private:
    BasicBlock *_bb;
};

class RegionCDGNode : public CDGNode
{
public:
    RegionCDGNode();
    void setCDS();

private:
};

#endif //CDG_CONSTRUCT_CDGNODE_H
