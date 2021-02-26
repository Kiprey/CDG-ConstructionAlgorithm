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

class ControlDependenceGraph : public GenericGraph<ControlDependenceNode,ControlDependenceEdge>
{
    // NULL
};


typedef GenericEdge<ControlDependenceNode> GenericCDEdgeTy;
class ControlDependenceEdge : public GenericCDEdgeTy
{
public:
    enum LabelType { T, F, None };
    ControlDependenceEdge(ControlDependenceNode* s, 
                       ControlDependenceNode* d, 
                       LabelType k)
        : GenericCDEdgeTy(s,d,k) {}
};


typedef GenericNode<ControlDependenceNode, ControlDependenceEdge> GenericCDNodeTy;
class ControlDependenceNode : public GenericCDNodeTy
{
public:
    enum NodeType { ControlNode, RegionNode };
    ControlDependenceNode(size_t id, NodeType ty) :  GenericCDNodeTy(id, ty) {  };
    
    void setBasicBlock(BasicBlock* bb) { _bb = bb; }
    BasicBlock* getBasicBlock() { return _bb; } 

private:
    BasicBlock* _bb;
};
