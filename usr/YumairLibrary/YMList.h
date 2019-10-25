#ifndef YM_LIST_H
#define YM_LIST_H

#define YMLIST_ENTRY(type) \
    type *next; type *prev

#define YMLIST_INIT_VALUE {0}

#define YMListInit(list) \
    (list)->next = (list); \
    (list)->prev = (list);

#define YMListFirst(list) ((list)->next == (list) ? YmNULL : (list)->next)

#define YMListLast(list) ((list)->prev == (list) ? YmNULL : (list)->prev)

#define __YMLIST_DOINIT(list) if(!((list)->next != YmNULL)){YMListInit(list);}

#define YMListAdd(list, node) \
    __YMLIST_DOINIT(list) \
    node->next = (list); \
    (list)->prev->next = node; \
    node->prev = (list)->prev; \
    (list)->prev = node;

#define YMListDel(node) \
if((node)->next != YmNULL) \
{ \
    (node)->prev->next = (node)->next; \
    (node)->next->prev = (node)->prev; \
    (node)->next = YmNULL; \
}

#define _YM_LIST_TMP_NODE(node, line) node##line
#define YM_LIST_TMP_NODE(node, line) _YM_LIST_TMP_NODE(node, line)

//YM_LIST_TMP_NODE 用于记录next节点，用于保证遍历过程中Del节点不会导致错误
#if 1
#define YMListForeach(list, node) \
    __YMLIST_DOINIT(list) \
    void *YM_LIST_TMP_NODE(node, __LINE__); \
for(node = (list)->next, YM_LIST_TMP_NODE(node, __LINE__) = node->next; \
    node != (list); node = YM_LIST_TMP_NODE(node, __LINE__), YM_LIST_TMP_NODE(node, __LINE__) = node->next)
#else
#define YMListForeach(list, node) \
for(node = (list)->next; node != (list); node = node->next)
#endif

#endif // YM_LIST_H
