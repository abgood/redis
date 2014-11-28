#include "redis.h"

// 创建对象
robj *createObject(int type, void *ptr) {
    robj *o = zmalloc(sizeof(*o));
    o->type = type;
    o->encoding = REDIS_ENCODING_RAW;
    o->ptr = ptr;
    o->refcount = 1;

    o->lru = server.lruclock;
    return o;
}

// 创建字符串对象
robj *createStringObject(char *ptr, size_t len) {
    return createObject(REDIS_STRING, sdsnewlen(ptr, len));
}
