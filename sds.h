#ifndef REDIS_SDS_H
#define REDIS_SDS_H

#define SDS_MAX_PREALLOC (1024 * 1024)

typedef char * sds;

struct sdshdr {
    unsigned int len;   // 字符串总长度
    unsigned int free;  // 字符串未使用长度
    char buf[];         // 字符串保存的位置
};

// sds已使用字符串长度
static inline size_t sdslen(const sds s) {
    /* s为buf, buf的地址减去2*sizeof(unsigned long)就是sh的地址
     * len|free|buf
     * sh      (sh->buf <==> const sds s) */
    struct sdshdr *sh = (void *)(s - (sizeof(struct sdshdr)));
    return sh->len;
}

// 返回可利用空间长度
static inline size_t sdsavail(const sds s) {
    struct sdshdr *sh = (void *)(s - (sizeof(struct sdshdr)));
    return sh->free;
}

sds sdsempty(void);
sds sdscatrepr(sds, const char *, size_t len);
sds sdscatprintf(sds, const char *, ...);
sds sdscat(sds, const char *);
sds sdsnew(const char *);
sds sdstrim(sds, const char *);
void sdsfree(sds);
void sdsrange(sds, int, int);
sds sdscatsds(sds s, const sds t);
sds *sdssplitlen(const char *, int, const char *, int, int *);
sds *sdssplitargs(const char *, int *);
sds sdsnewlen(const void *, size_t);

#endif
