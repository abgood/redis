#include "redis.h"

#define PREFIX_SIZE (sizeof(size_t))

// 更新zmalloc状态add, 多线程条件下加锁处理
#define update_zmalloc_stat_add(__n) do {   \
    pthread_mutex_lock(&used_memory_mutex); \
    used_memory += (__n);   \
    pthread_mutex_unlock(&used_memory_mutex); \
} while (0)

// 更新zmalloc状态sub, 多线程条件下加锁处理
#define update_zmalloc_stat_sub(__n) do {   \
    pthread_mutex_lock(&used_memory_mutex); \
    used_memory -= (__n);   \
    pthread_mutex_unlock(&used_memory_mutex);   \
} while (0)

// 更新内存占用量的统计
#define update_zmalloc_stat_alloc(__n) do { \
    size_t _n = (__n);  \
    if (_n & (sizeof(long) - 1)) _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
    if (zmalloc_thread_safe) {  \
        update_zmalloc_stat_add(_n);    \
    } else {    \
        used_memory += _n;  \
    }   \
} while (0)

// 释放内存并重置内存占用量
#define update_zmalloc_stat_free(__n) do {  \
    size_t _n = (__n);  \
    if (_n & (sizeof(long) - 1)) _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
    if (zmalloc_thread_safe) {  \
        update_zmalloc_stat_sub(_n);    \
    } else {    \
        used_memory -= _n;  \
    }   \
} while (0)

static int zmalloc_thread_safe = 0;
static size_t used_memory = 0;
pthread_mutex_t used_memory_mutex = PTHREAD_MUTEX_INITIALIZER;

// default out of memory
static void zmalloc_default_oom(size_t size) {
    fprintf(stderr, "zmalloc: Out of memory trying to allocate %zu bytes\n", size);
    fflush(stderr);
    abort();
}

static void (*zmalloc_oom_handler)(size_t) = zmalloc_default_oom;

// 自动分配线程安全
void zmalloc_enable_thread_safeness(void) {
    zmalloc_thread_safe = 1;
}

// 设置自动分配函数的句柄(out of memory)
void zmalloc_set_oom_handler(void (*oom_handler)(size_t)) {
    zmalloc_oom_handler = oom_handler;
}

// 分配空间:sizeof(struct) + initlen + 1
void *zmalloc(size_t size) {
    void *ptr = malloc(size + PREFIX_SIZE);
    // expect out of memory
    if (!ptr) zmalloc_oom_handler(size);

    // 内存块头部保存内存块大小
    *((size_t *)ptr) = size;
    update_zmalloc_stat_alloc(size + PREFIX_SIZE);
    return (char *)ptr + PREFIX_SIZE;
}

// 重新分配
void *zrealloc(void *ptr, size_t size) {
    void *realptr;
    size_t oldsize;
    void *newptr;

    if (ptr == NULL) return zmalloc(size);
    // 原字符串长度, 字符串头部
    realptr = (char *)ptr - PREFIX_SIZE;
    oldsize = *((size_t *)realptr);
    newptr = realloc(realptr, size + PREFIX_SIZE);
    if (!newptr) zmalloc_oom_handler(size);

    *((size_t *)newptr) = size;
    // 释放之前的空间
    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(size);
    return (char *)newptr + PREFIX_SIZE;
}

// 释放空间
void zfree(void *ptr) {
    void *realptr;
    size_t oldsize;

    if (ptr == NULL) return;
    realptr = (char *)ptr - PREFIX_SIZE;
    oldsize = *((size_t *)realptr);
    update_zmalloc_stat_free(oldsize + PREFIX_SIZE);
    free(realptr);
}

// 字符串复制
char *zstrdup(const char *s) {
    size_t l = strlen(s) + 1;
    char *p = zmalloc(l);

    memcpy(p, s, l);
    return p;
}
