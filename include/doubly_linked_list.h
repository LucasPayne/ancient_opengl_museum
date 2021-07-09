#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H
/*================================================================================
    Generic doubly-linked lists.

    usage:
        The list contains pointers to the first and last nodes. Each node is a struct
        whose top two entries are pointers to nodes, the previous and the next. This
        "inheritence" is achieved by casting.

        For example, putting objects into a doubly linked list looks like this:
            struct ThingNode_s;
            typedef struct ThingNode_s {
                struct ThingNode_s *prev;
                struct ThingNode_s *next;
                Thing thing;
            } ThingNode;
        Nodes are allocated on the heap, but this must be done by the user before
        adding a node to the list. Nodes live only in the list, so are freed when
        they are removed.
================================================================================*/
typedef struct DLNode_s {
    struct DLNode_s *prev;
    struct DLNode_s *next;
} DLNode;
typedef struct DLList_s {
    DLNode *first;
    DLNode *last;
} DLList;
DLList dl_new(void);
#define dl_add(LIST,NODE)\
    ___dl_add((DLList *) ( LIST ), (DLNode *) ( NODE ))
DLNode *___dl_add(DLList *list, DLNode *node);
#define dl_remove(LIST,NODE)\
    ___dl_remove((DLList *) ( LIST ), (DLNode *) ( NODE ))
void ___dl_remove(DLList *list, DLNode *node);

#endif // DOUBLY_LINKED_LIST_H
