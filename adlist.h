#ifndef REDIS_ADLIST_H
#define REDIS_ADLIST_H

// 循环链表, 值
typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

typedef struct list {
    listNode *head;
    listNode *tail;
    // 返回指针类型的函数指针
    void *(*dup)(void *ptr);
    // 函数指针
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);
    unsigned long len;
} list;

list *listCreate(void);

#endif
