#include "CDG.h"

ControlDependenceSetElement::ControlDependenceSetElement(NodeID _id, CDGEdge::LabelType ty)
    : _CDG_ID(_id), _label(ty) {}

bool ControlDependenceSetElement::operator<(const ControlDependenceSetElement& e) const
{
    if(_CDG_ID < e._CDG_ID)   
        return true;
    else if(_CDG_ID == e._CDG_ID) 
        return false;
    else
        return _label < e._label;
}

bool ControlDependenceSetElement::operator==(const ControlDependenceSetElement& e) const
{
    return _CDG_ID == e._CDG_ID && _label == e._label;
}

NodeID ControlDependenceSetElement::getNodeID()
{
    return _CDG_ID;
}

CDGEdge::LabelType ControlDependenceSetElement::getLabel()
{
    return _label;
}

ControlDependenceSet operator& (const ControlDependenceSet& cds1, const ControlDependenceSet& cds2)
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