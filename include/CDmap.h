#ifndef CDMAP_H
#define CDMAP_H

#include <map>
#include <set>

#include "CDG.h"

using namespace SVF;
using namespace llvm;
using namespace std;

// 某个节点依赖集所存放的单个元素。以 { nodeID, edgeLabel } 为一个整体存放
// 例如依赖 { 1, F }
struct ControlDependenceSetElement{
    int CDG_ID;
    enum { T, F, None } label;
    // 排序时优先按照 节点编号 排序，例如 {1, T} < {1, F} < {2, T}
    bool operator<(const ControlDependenceSetElement& e) const
    {
        if(CDG_ID < e.CDG_ID)   
            return true;
        else if(CDG_ID == e.CDG_ID) 
            return false;
        else
            return label < e.label;
    }
    bool operator==(const ControlDependenceSetElement& e) const
    {
        return CDG_ID == e.CDG_ID && label == e.label;
    }
};

// 设置数据结构 set
typedef set<ControlDependenceSetElement> ControlDependenceSet;
// 实现两个 ControlDependenceSet 取交集的函数
ControlDependenceSet operator& (ControlDependenceSet& cds1, ControlDependenceSet& cds2);
// 设置数据结构 map
typedef map<CDGNode, ControlDependenceSet> ControlDependenceMap;
// 设置缩写
typedef ControlDependenceMap CDMap;
typedef ControlDependenceSetElement CDSetElem;
typedef ControlDependenceSet CDSet; 

#endif