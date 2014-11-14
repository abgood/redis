#ifndef REDIS_SDS_H
#define REDIS_SDS_H

typedef char * sds;

struct sdshdr {
    unsigned int len;   // 字符串总长度
    unsigned int free;  // 字符串未使用长度
    char buf[];         // 字符串保存的位置
};

sds sdsempty(void);
sds sdscatrepr(sds, const char *, size_t len);

#endif
