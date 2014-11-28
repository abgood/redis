#include "redis.h"

list *listCreate(void) {
    struct list *list;

    // list指针分配空间
    if ((list = zmalloc(sizeof(*list))) == NULL) {
        return NULL;
    }

    // init
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;

    return list;
}
