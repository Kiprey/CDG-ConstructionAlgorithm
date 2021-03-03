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
    ControlDependenceNode(NodeID i,NodeType ty );
    void setBasicBlock(BasicBlock *bb);
    BasicBlock *getBasicBlock();

private:
    BasicBlock *_bb;
};

class ControlCDGNode : public CDGNode
{
public:
    ControlCDGNode(NodeID id,BasicBlock *bb);

private:
};

class RegionCDGNode : public CDGNode
{
public:
    RegionCDGNode(NodeID id,BasicBlock *bb);

private:
};

#endif //CDG_CONSTRUCT_CDGNODE_H
