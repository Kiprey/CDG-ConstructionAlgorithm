//
// Created by wz on 2021/3/2.
//

#ifndef CDG_CONSTRUCT_CDGEDGE_H
#define CDG_CONSTRUCT_CDGEDGE_H
using namespace SVF;
using namespace llvm;
using namespace std;

class ControlDependenceEdge;
class ControlDependenceNode;

typedef ControlDependenceNode CDGNode;
typedef ControlDependenceEdge CDGEdge;
typedef GenericEdge<ControlDependenceNode> GenericCDEdgeTy;
class ControlDependenceEdge : public GenericCDEdgeTy
{
public:
    typedef GenericNode<CDGNode,CDGEdge>::GEdgeSetTy CDGEdgeSetTy;
    typedef CDGEdge::CDGEdgeSetTy::iterator iterator;
    typedef CDGEdge::CDGEdgeSetTy::const_iterator const_iterator;

    typedef int LabelType;
    LabelType label;//-1==default,-2==None
    ControlDependenceEdge(ControlDependenceNode *s,
                          ControlDependenceNode *d,
                          LabelType k=-2);
};

#endif //CDG_CONSTRUCT_CDGEDGE_H
