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
// 设置数据结构 map
typedef map<CDGNode, ControlDependenceSet> ControlDependenceMap;
// 设置缩写
typedef ControlDependenceMap CDMap;
typedef ControlDependenceSetElement CDE;
typedef ControlDependenceSet CDS; 

// 实现两个 ControlDependenceSet 取交集的函数
ControlDependenceSet operator& (ControlDependenceSet& cds1, ControlDependenceSet& cds2) 
{
    ControlDependenceSet result;
    /* testcase: 1, 2, 3, 5, 8 | 2, 4, 5, 6, 8*/
    auto iter1 = cds1.begin();
    auto iter2 = cds2.begin();
    while(iter2 != cds2.end())
    {
        // 如果lhs全部遍历完成， 或者lhs > rhs，则 rhsIter++
        if(iter1 == cds1.end() || *iter2 < *iter1)
            iter2++;
        // 如果当前遍历到的 lhs == rhs，则加入result
        else if(*iter1 == *iter2)
        {
            iter2++;
            result.insert(*iter1);
        }
        // 否则， lhs < rhs,继续向下遍历 lhs
        else
            iter1++;
    }
    return result;
}   

#endif