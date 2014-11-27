#ifndef REDIS_ADLIST_H
#define REDIS_ADLIST_H

typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

typedef struct list {
    listNode *head;
    listNode *tail;

    void *(*dup)(void *ptr);
    void *(*free)(void *ptr);
    int (*match)(void *ptr, void *key);
    unsigned long len;
} list;

list *listCreate(void);

#endif
