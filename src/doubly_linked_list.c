/*--------------------------------------------------------------------------------
    Implementation of the doubly linked list module.
--------------------------------------------------------------------------------*/
#include <stdlib.h>
#include "doubly_linked_list.h"

DLList dl_new(void)
{
    DLList dl;
    dl.first = NULL;
    dl.last = NULL;
    return dl;
}
DLNode *___dl_add(DLList *list, DLNode *node)
{
    if (list->first == NULL) {
        list->first = node;
        list->last = node;
        return node;
    }
    node->prev = list->last;
    node->next = NULL;
    list->last->next = node;
    list->last = node;
    return node;
}
void ___dl_remove(DLList *list, DLNode *node)
{
    if (node == list->first && node == list->last) {
        list->first = NULL;
        list->last = NULL;
    } else if (node == list->first) {
        list->first = list->first->next;
        list->first->prev = NULL;
    } else if (node == list->last) {
        list->last = list->last->prev;
        list->last->next = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    free(node);
}
