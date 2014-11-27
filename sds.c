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

// sds扩展空间
sds sdsMakeRoomFor(sds s, size_t addlen) {
    struct sdshdr *sh, *newsh;
    size_t free = sdsavail(s);
    size_t len, newlen;

    // 有多的剩余空间
    if (free >= addlen) return s;
    len = sdslen(s);
    sh = (void *)(s - sizeof(struct sdshdr));
    // 新的总长度
    newlen = len + addlen;

    if (newlen < SDS_MAX_PREALLOC) {
        newlen *= 2;
    } else {
        newlen += SDS_MAX_PREALLOC;
    }

    // 扩展空间
    newsh = zrealloc(sh, sizeof(struct sdshdr) + newlen + 1);
    if (newsh == NULL) return NULL;

    newsh->free = newlen - len;
    return newsh->buf;
}

// 追加len个'\"'字符到s后面
sds sdscatlen(sds s, const void *t, size_t len) {
    struct sdshdr *sh;
    size_t curlen = sdslen(s);

    s = sdsMakeRoomFor(s, len);
    if (s == NULL) return NULL;
    sh = (void *)(s - (sizeof(struct sdshdr)));
    // 拷贝新串到源串
    memcpy(s + curlen, t, len);
    // reset used and free
    sh->len = curlen + len;
    sh->free = sh->free - len;
    s[curlen + len] = '\0';

    return s;
}

// 追加并在末尾添加空字符串
sds sdscat(sds s, const char *t) {
    return sdscatlen(s, t, strlen(t));
}

// 追加字符串,转义处理并返回新串指针
sds sdscatrepr(sds s, const char *p, size_t len) {
    s = sdscatlen(s, "\"", 1);

    while (len--) {
        switch (*p) {
        case '\\':
        case '"':
            // 格式化字符串并追加到s
            s = sdscatprintf(s, "\\%c", *p);
            break;
        case '\n': s = sdscatlen(s, "\\n", 2); break;
        case '\r': s = sdscatlen(s, "\\r", 2); break;
        case '\t': s = sdscatlen(s, "\\t", 2); break;
        case '\a': s = sdscatlen(s, "\\a", 2); break;
        case '\b': s = sdscatlen(s, "\\b", 2); break;
        default:
            if (isprint(*p)) {
                s = sdscatprintf(s, "%c", *p);
            } else {
                s = sdscatprintf(s, "\\x%02x", (unsigned char)*p);
            }
            break;
        }
        p++;
    }

    return sdscatlen(s, "\"", 1);
}

// fromat string to s by va_list
sds sdscatvprintf(sds s, const char *fmt, va_list ap) {
    va_list cpy;
    char staticbuf[1024], *buf = staticbuf, *t;
    size_t buflen = strlen(fmt) * 2;

    // 分配空间
    if (buflen > sizeof(staticbuf)) {
        buf = zmalloc(buflen);
        if (buf == NULL) return NULL;
    } else {
        buflen = sizeof(staticbuf);
    }

    // 2 * buffer_size
    // 每次循环扩大是以前的2倍
    while (1) {
        buf[buflen - 2] = '\0';
        // copy va_list
        va_copy(cpy, ap);
        vsnprintf(buf, buflen, fmt, cpy);
        va_end(ap);

        if (buf[buflen - 2] != '\0') {
            if (buf != staticbuf) zfree(buf);
            buflen *= 2;
            buf = zmalloc(buflen);
            if (buf == NULL) return NULL;
            continue;
        }
        break;
    }

    // get string
    t = sdscat(s, buf);
    if (buf != staticbuf) zfree(buf);
    return t;
}

// 追加格式化字符串到s
sds sdscatprintf(sds s, const char *fmt, ...) {
    va_list ap;
    char *t;
    va_start(ap, fmt);
    // s + 格式化 返回 t
    t = sdscatvprintf(s, fmt, ap);
    va_end(ap);
    return t;
}

// 创建一个新的c类型字符串
sds sdsnew(const char *init) {
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}

// 从sds字符串左右两端分别移除cset里出现的字符
sds sdstrim(sds s, const char *cset) {
    struct sdshdr *sh =(void *)(s - (sizeof(struct sdshdr)));
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s + sdslen(s) - 1;
    // from left to right
    while (sp <= end && strchr(cset, *sp)) sp++;
    // from right to left
    while (ep > start && strchr(cset, *ep)) ep--;
    // sp > ep: sp在ep的后面
    len = (sp > ep) ? 0 : ((ep - sp) + 1);
    // 拷贝数据,可处理重叠数据问题
    if (sh->buf != sp) memmove(sh->buf, sp, len);
    sh->buf[len] = '\0';
    sh->free = sh->free + (sh->len - len);
    sh->len = len;
    return s;
}

// 释放sds字符串
void sdsfree(sds s) {
    if (s == NULL) return;
    zfree(s - sizeof(struct sdshdr));
}

// 获取区间内的字符串
void sdsrange(sds s, int start, int end) {
    struct sdshdr *sh = (void *)(s - sizeof(struct sdshdr));
    size_t newlen, len = sdslen(s);

    if (len == 0) return;
    if (start < 0) {
        start = len + start;
        if (start < 0) start = 0;
    }

    if (end < 0) {
        end = len + end;
        if (end < 0) end = 0;
    }

    newlen = (start > end) ? 0 : (end - start) + 1;
    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len - 1;
            newlen = (start > end) ? 0 : (end - start) + 1;
        }
    } else {
        start = 0;
    }

    if (start && newlen) memmove(sh->buf, sh->buf + start, newlen);
    sh->buf[newlen] = 0;
    sh->free = sh->free + (sh->len - newlen);
    sh->len = newlen;
}

// sds追加sds
sds sdscatsds(sds s, const sds t) {
    return sdscatlen(s, t, sdslen(t));
}

// 分隔len长度的字符串
// 指针的指针保存第行数据
sds *sdssplitlen(const char *s, int len, const char *sep, int seplen, int *count) {
    int elements = 0, slots = 5, start = 0, j;
    // 指针的指针
    sds *tokens;

    if (seplen < 1 || len < 0) return NULL;

    // 带内存大小头的内存分配
    tokens = zmalloc(sizeof(sds) * slots);
    if (tokens == NULL) return NULL;

    // tokens为NULL
    if (len == 0) {
        *count = 0;
        return tokens;
    }

    for (j = 0; j < (len - (seplen + 1)); j++) {
        // 确保有下一个元素和最后一个元素的空间
        if (slots < elements + 2) {
            sds *newtokens;

            slots *= 2;
            // 重新增加空间分配
            newtokens = zrealloc(tokens, sizeof(sds) * slots);
            if (newtokens == NULL) goto cleanup;
            tokens = newtokens;
            // printf("%d, %d\n", slots, elements);
        }

        // 查询分隔符
        if ((seplen == 1 && *(s + j) == sep[0]) || (memcmp(s + j, sep, seplen) == 0)) {
            tokens[elements] = sdsnewlen(s + start, j - start);
            if (tokens[elements] == NULL) goto cleanup;

            // printf("%s\n", tokens[elements]);

            // 行数
            elements++;
            // 下一行开头
            start = j + seplen;
            // 跳过分隔符, j在for里会先自加
            j = j + seplen - 1;
        }
    }

    // 添加最后的元素
    tokens[elements] = sdsnewlen(s + start, len - start);
    if (tokens[elements] == NULL) goto cleanup;
    elements++;
    *count = elements;
    return tokens;

// 释放
cleanup:
    {
        int i;
        // 内存释放溢出, elements == j(len)
        for (i = 0; i < elements; i++) sdsfree(tokens[i]);
        zfree(tokens);
        *count = 0;
        printf("free\n");
        return NULL;
    }
}

// 测试16进制
int is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F');
}

// 转换16进制成整形0~15
int hex_digit_to_int(char c) {
    switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
    }
}

// 分隔每行配置成参数数组
sds *sdssplitargs(const char *line, int *argc) {
    const char *p = line;
    char *current = NULL;
    char **vector = NULL;

    *argc = 0;
    while (1) {
        // 跳过空行
        while (*p && isspace(*p)) p++;
        if (*p) {
            int inq = 0;
            int insq = 0;
            int done = 0;

            if (current == NULL) current = sdsempty();
            while (!done) {
                // 双引号
                if (inq) {
                    // 16进制
                    if (*p == '\\' && *(p + 1) == 'x' &&
                            is_hex_digit(*(p + 2)) &&
                            is_hex_digit(*(p + 3))) {
                        unsigned char byte;

                        byte = (hex_digit_to_int(*(p + 2)) * 16) +
                            hex_digit_to_int(*(p + 3));
                        current = sdscatlen(current, (char *)&byte, 1);
                        p += 3;
                    } else if (*p == '\\' && *(p + 1)) {
                        // 特殊字符
                        char c;

                        p++;
                        switch (*p) {
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'b': c = '\b'; break;
                        case 'a': c = '\a'; break;
                        default: c = *p; break;
                        }
                        current = sdscatlen(current, &c, 1);
                    } else if (*p == '"') {
                        // 空字符
                        // a space or nothing at all
                        if (*(p + 1) && !isspace(*(p + 1))) goto err;
                        done = 1;
                    } else if (!*p) {
                        goto err;
                    } else {
                        current = sdscatlen(current, p, 1);
                    }
                } else if (insq) {
                    // 单引号
                    if (*p == '\\' && *(p + 1) == '\'') {
                        p++;
                        current = sdscatlen(current, "'", 1);
                    } else if (*p == '\'') {
                        if (*(p + 1) && !isspace(*(p + 1))) goto err;
                        done = 1;
                    } else if (!*p) {
                        goto err;
                    } else {
                        current = sdscatlen(current, p, 1);
                    }
                } else {
                    switch (*p) {
                        case ' ':
                        case '\n':
                        case '\r':
                        case '\t':
                        case '\0':
                            done = 1;
                            break;
                        case '"':
                            inq = 1;
                            break;
                        case '\'':
                            insq = 1;
                            break;
                        default:
                            current = sdscatlen(current, p, 1);
                            break;
                    }
                }
                if (*p) p++;
            }
            // 添加token到vector
            vector = zrealloc(vector, ((*argc) + 1) * sizeof(char *));
            vector[*argc] = current;
            (*argc)++;
            current = NULL;
        } else {
            if (vector == NULL) vector = zmalloc(sizeof(void *));
            return vector;
        }
    }

err:
    while ((*argc)--) {
        sdsfree(vector[*argc]);
    }
    zfree(vector);
    if (current) sdsfree(current);
    *argc = 0;
    return NULL;
}
