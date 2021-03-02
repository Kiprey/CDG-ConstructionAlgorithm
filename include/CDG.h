#ifndef CDG_H
#define CDG_H

#include <map>

#include "Graphs/GenericGraph.h"
#include "Graphs/SVFG.h"
#include "CDmap.h"
#include "CDGEdge.h"
#include "CDGNode.h"

using namespace SVF;
using namespace llvm;
using namespace std;

class ControlDependenceGraph;
class ControlDependenceNode;

typedef ControlDependenceGraph CDG;

// 设置ControlDependenceGraph 的类型
class ControlDependenceGraph : public GenericGraph<ControlDependenceNode,ControlDependenceEdge>
{
public:
    typedef DomTreeNodeBase<llvm::BasicBlock>* DTNodeTy;
    typedef struct {
        DTNodeTy A;
        DTNodeTy B;
        DTNodeTy L;
        NodeID TF;
    }DepenTupleTy;
    typedef Set<DepenTupleTy*> DepenSSetTy;
    typedef vector<DTNodeTy> DepenVecTy;

public:
    ControlDependenceGraph();

    void initCDG(SVFFunction *fun );//construct initial CDG
    void findSSet(ICFGNode* entryNode,Set<const ICFGNode*> &visited,DepenSSetTy &setS);
    void buildinitCDG(DepenSSetTy S);
    u32_t icfgOutEdgeNum(ICFGNode* iNode);
    void findPathL2B(DepenSSetTy S,vector <DTNodeTy> &P);
    void findPathA2B(DTNodeTy A,DepenSSetTy S,vector <DTNodeTy> &P);
    void handleDepenVec(DepenTupleTy* LB,vector <DTNodeTy> &P);
    CDGEdge::LabelType lable2bool(NodeID TF);


    void addCDGEdge(CDGNode *s, CDGNode *d, CDGEdge::LabelType l);

    inline CDGNode* getCDGNode(NodeID id);
    inline bool hasCDGNode(NodeID id);
    inline void removeCDGNode(NodeType* node);
    NodeID getNodeIDFromBB(BasicBlock* bb);
private:
    // BasicBlock* -> CDG Node ID
    map<BasicBlock*, NodeID> _bb2CDGNodeID;
    PostDominatorTree* PDT = nullptr;
    CDMapTy CDMap;
    inline void addCDGNode(NodeType* node);
    inline CDGNode* addControlCDGNode(BasicBlock* nbb);
    inline CDGNode* addRegionCDGNode();
};

#endif
