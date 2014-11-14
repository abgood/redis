#include "redis.h"

// 创建initlen字符串
// init为初始化字符串,initlen字符串长度
sds sdsnewlen(const void *init, size_t initlen) {
    struct sdshdr *sh = NULL;

    if (init) {
        sh = zmalloc(sizeof(struct sdshdr) + initlen + 1);
    } else {
        printf("zcalloc\n");
    }

    if (!sh) return NULL;

    // 保存字符串的属性
    sh->len = initlen;
    sh->free = 0;

    // copy init to sh->buf
    if (initlen && init) {
        memcpy(sh->buf, init, initlen);
    }
    sh->buf[initlen] = '\0';
    return (char *)sh->buf;
}

// empty string
sds sdsempty(void) {
    return sdsnewlen("", 0);
}

// sds已使用字符串长度
static inline size_t sdslen(const sds s) {
    /* s为buf, buf的地址减去2*sizeof(unsigned long)就是sh的地址
     * len|free|buf
     * sh      (sh->buf <==> const sds s) */
    struct sdshdr *sh = (void *)(s - (sizeof(struct sdshdr)));
    return sh->len;
}

// 追加len个'\"'字符到s后面
sds sdscatlen(sds s, const void *t, size_t len) {
    struct sdshdr *sh;
    size_t curlen = sdslen(s);

    s = sdsMakeRoomFor(s, len);

    return s;
}

// 追加字符串,转义处理并返回新串指针
sds sdscatrepr(sds s, const char *p, size_t len) {
    s = sdscatlen(s, "\"", 1);
}
