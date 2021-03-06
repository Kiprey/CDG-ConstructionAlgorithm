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
    
    /**
     *  由于受到 switch 语句的影响，Label 可以有多个类型（不只是 T/F/None)
     *  还包括了 switch 中的 各个分支 value 以及 Default
     *  为了便于代码可读，设置 Default 为 -2, None 为 -1
     *  而对于 T/F 以及 switch 中的各个分支 value，  将其转换成 int 后成为新的 LabelType
     */
    typedef int LabelType;
    LabelType label;
    static const LabelType LabelType_None = -1;
    static const LabelType LabelType_Default = -2;

    ControlDependenceEdge(ControlDependenceNode *s,
                          ControlDependenceNode *d,
                          LabelType k = LabelType_None);
};

#endif //CDG_CONSTRUCT_CDGEDGE_H
