#include <stack>

#include "CDG.h"

typedef list<CDSetElemTy> CDElemPathTy;

list<CDElemPathTy> traverseEdgesInFunc(CDGEdge* edge, 
                                CDElemPathTy curr_path, 
                                CDGNode* A,
                                set<CDGNode*> visited,
                                CDGNode** ret_entry_node)
{
    CDGNode* src = edge->getSrcNode();
    // 加个 visited 集合防止循环
    if(visited.find(src) != visited.end())
        return list<CDElemPathTy>();
    visited.insert(src);
    
    list<CDElemPathTy> result;
    // 如果当前节点是 controlNode,则 将该边加入 curr_path
    if(ControlCDGNode::classof(src))
    {
        CDSetElemTy cdet = { edge->getSrcID(), edge->getLabel() };
        curr_path.push_back(cdet);
    }

    // 在处理完 curr_path.push_back 的操作后,再来判断 目标节点是否为 A
    if(src == A)
        result.push_back(curr_path);
    else
    {
        // 对于当前节点来说, 将当前 src 节点的 inEdge 中的直接边 进行遍历
        // 并判断一下当前节点是否为 Entry 节点
        bool isEntry = true;
        for(auto iter = src->InEdgeBegin(); iter != src->InEdgeEnd(); iter++)
        {
            CDGEdge* edge = *iter;
            if (SVFUtil::isa<IntraCFGEdge>((ICFGEdge*)edge))
            {
                isEntry = false;
                list<CDElemPathTy> single_path = traverseEdgesInFunc(edge, curr_path, A, visited, ret_entry_node);
                for(auto listIter = single_path.begin(); listIter != single_path.end(); listIter++)
                    result.push_back(*listIter);
            }
        }
        // 如果识别出 Entry 节点, 则将其返回给上层函数
        if(isEntry)
        { 
            if(ret_entry_node != nullptr)
                *ret_entry_node = src;
        }
    }
    // 这里不需要对 curr_path 和 visited 进行清除最后一步操作
    // 因为这两个参数都是局部变量
    return result;
}

/**
 * @brief                   获取过程内 A -> B 所需要的控制依赖
 * @param A                 源节点的指针
 * @param B                 目标节点的指针
 * @param ret_entry_node    指向保存 entry node 指针的指针
 * @return                  返回一条从 A -> B 的控制依赖路径
 * @note                    注意如果 AB 两个节点相同,则返回空列表
 * 
 * @todo                    这个函数的递归不太完美,有相当一部分代码是重复的,可能要完善一下(具体看情况)
 */
list<CDElemPathTy> find_A_2_B_in_Func(
    CDGNode* A, 
    CDGNode* B, 
    CDGNode** ret_entry_node = nullptr)
{
    list<CDElemPathTy> result;
    // 声明单次的 path, 这里使用 list 以保证有序性
    
    // 回溯 B 到 entry 的所有依赖路径, 判断 A 是否在某条依赖路径上
    // 将节点 B 的相邻边加入worklist中
    CDElemPathTy curr_path;
    set<CDGNode*> visited;
    for(auto iter = B->InEdgeBegin(); iter != B->InEdgeEnd(); iter++)
    {
        CDGEdge* edge = *iter;
        list<CDElemPathTy> single_path = traverseEdgesInFunc(edge, curr_path, A, visited, ret_entry_node);
        for(auto listIter = single_path.begin(); listIter != single_path.end(); listIter++)
            result.push_back(*listIter);
    }
    
    return result;
}


list<CDElemPathTy> traverseEdgesBetweenFunc(
                                CDGNode* entry, 
                                CDElemPathTy curr_path, 
                                CDGNode* A,
                                set<CDGNode*> visited)
{
    list<CDElemPathTy> result;
    for(auto iter = entry->InEdgeBegin(); iter != entry->InEdgeEnd(); iter++)
    {
        // 遍历各个间接边
        CDGEdge* edge = *iter;
        // 获取该间接边所对应的 callsite 节点
        CDGNode* callsite = edge->getSrcNode();
        // 判断当前callsite 是否与 A 位于同一个函数
        CDGNode *entry_in_callsite_func;
        find_A_2_B_in_Func(callsite, callsite, &entry_in_callsite_func);
        // 如果是,则进入函数内查询
//        if(entry_in_A_func == entry_in_callsite_func)
//        {
//            list<CDElemPathTy> A_2_callsite_path = find_A_2_B_in_Func(A, callsite);
//            // 返回
//        }
//        else
//        {
//            //如果不是,则查找 callsite 至 entry_in_callsite_func 的路径
//            list<CDElemPathTy> callsiteEntry_2_callsite_path
//                = find_A_2_B_in_Func(entry_in_callsite_func, callsite);
//        }
        
        
    }
    return result;
}
/**
 * @brief 过程间的 A-> B 控制依赖 查询
 * @warning 小心递归这样的情况
 * @todo 内存爆炸
 * @todo 仍然还是
 */

list<CDElemPathTy> find_A_2_B_between_Funcs(CDGNode* A, CDGNode* B)
{
    // 首先,获取 A 节点 和 B 节点所属的 Entry 节点
    CDGNode *entry_in_A_func, *entry_in_B_func;
    find_A_2_B_in_Func(A, A, &entry_in_A_func);
    find_A_2_B_in_Func(B, B, &entry_in_B_func);
    // 这里需要判断一下,A 和 B 是否位于同一个 func 中
    // 如果是,则进入函数内查询
    if(entry_in_A_func == entry_in_B_func)
        return find_A_2_B_in_Func(A, B);
    // 否则,接下来是函数间的查询
    //  1. 获取 B 到对应 Entry 的路径
    list<CDElemPathTy> A2B_path = find_A_2_B_in_Func(entry_in_B_func, B);
    //  2. 递归获取 子 entry 到父 A 节点的路径
    list<CDElemPathTy> result;
    CDElemPathTy curr_path;
    set<CDGNode*> visited;
    for(auto iter = entry_in_B_func->InEdgeBegin(); iter != entry_in_B_func->InEdgeEnd(); iter++)
    {
        // 遍历各个间接边
        CDGEdge* edge = *iter;
        // 获取该间接边所对应的 callsite 节点
        CDGNode* callsite = edge->getSrcNode();
        // 判断当前callsite 是否与 A 位于同一个函数
        CDGNode *entry_in_callsite_func;
        find_A_2_B_in_Func(callsite, callsite, &entry_in_callsite_func);
        // 如果是,则进入函数内查询
        if(entry_in_A_func == entry_in_callsite_func)
        {
            list<CDElemPathTy> A_2_callsite_path = find_A_2_B_in_Func(A, callsite);
            /// @todo 返回            
        }
        else
        {
            //如果不是,则查找 callsite 至 entry_in_callsite_func 的路径
            list<CDElemPathTy> callsiteEntry_2_callsite_path 
                = find_A_2_B_in_Func(entry_in_callsite_func, callsite);
            /// @todo
        }
        
        
    }
    return result;
}