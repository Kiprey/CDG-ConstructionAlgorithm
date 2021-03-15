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
        RegionNode,
        FunEntryNode
    };
    ControlDependenceNode(NodeID i, NodeType ty);
    ~ControlDependenceNode();
    
    void setBasicBlock(const BasicBlock *bb);
    const BasicBlock *getBasicBlock();

    /// Overloading operator << for dumping CDG node ID
    friend raw_ostream &operator<<(raw_ostream &o, const CDGNode &node)
    {
        o << node.toString();
        return o;
    }

    virtual const std::string toString() const;

    /// Return the function of this CDGNode
    inline const SVFFunction* getFun() const
    {
        return fun;
    }
    /// Return the function of this CDGNode
    inline const BasicBlock* getBB() const
    {
        return _bb;
    }
protected:
    const BasicBlock *_bb;
    const SVFFunction* fun;
};

class ControlCDGNode : public CDGNode
{
public:
    ControlCDGNode(NodeID id,const BasicBlock *bb);

    //@}
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const ControlCDGNode *node)
    {
        return true;
    }
    static inline bool classof(const CDGNode *node)
    {
        return node->getNodeKind() == ControlNode;
    }

    static inline bool classof(const GenericCDNodeTy *node)
    {
        return node->getNodeKind() == ControlNode;
    }
    //@}

    virtual const std::string toString() const;

private:
};

class RegionCDGNode : public CDGNode
{
public:
    RegionCDGNode(NodeID id);

    //@}
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const ControlCDGNode *node)
    {
        return true;
    }
    static inline bool classof(const CDGNode *node)
    {
        return node->getNodeKind() == RegionNode;
    }

    static inline bool classof(const GenericCDNodeTy *node)
    {
        return node->getNodeKind() == RegionNode;
    }
    //@}

    virtual const std::string toString() const;

private:
};
/*!
 * Function entry CDGNode of a function
 */
class FunEntryCDGNode : public CDGNode
{

public:
    FunEntryCDGNode(NodeID id, const SVFFunction* f,const BasicBlock* bb);

    ///Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{

   static inline bool classof(const FunEntryCDGNode *node)
    {
        return true;
    }

    static inline bool classof(const CDGNode *node)
    {
        return node->getNodeKind() == FunEntryNode;
    }

    static inline bool classof(const GenericCDNodeTy *node)
    {
        return node->getNodeKind() == FunEntryNode;
    }
    //@}

    const virtual std::string toString() const;
};

#endif //CDG_CONSTRUCT_CDGNODE_H
