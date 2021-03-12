//
// Created by wz on 2021/3/12.
//

#ifndef CDG_CONSTRUCT_CDGBUILDER_H
#define CDG_CONSTRUCT_CDGBUILDER_H

#include "CDG.h"


class CDGBuilder{

public:
    typedef std::vector<const Instruction*> InstVec;
    typedef Set<const Instruction*> BBSet;

    typedef FIFOWorkList<const Instruction*> WorkList;
private:
    CDG* cdg;

public:
    CDGBuilder(CDG* c):cdg(c){};
    void build(SVFModule* svfModule);
    void processCallsite(const SVFFunction*  fun, WorkList& worklist);
    void handleCallSite(const Instruction *inst);
    void addInterCDGEdge(CDGNode* srcNode,CDGNode* dstNode);
    CDGNode* getOrAddControlCDGNode(const llvm::BasicBlock* bb);
};

#endif //CDG_CONSTRUCT_CDGBUILDER_H
