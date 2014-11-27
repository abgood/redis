#include "redis.h"

sds getAbsolutePath(char *filename) {
    char cwd[1024];
    sds abspath;
    sds relpath = sdsnew(filename);

    relpath = sdstrim(relpath, " \r\n\t");
    // path已经是绝对路径
    if (relpath[0] == '/') return relpath;

    // 相对路径处理
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        sdsfree(relpath);
        return NULL;
    }

    abspath = sdsnew(cwd);
    if (sdslen(abspath) && abspath[sdslen(abspath) - 1] != '/') {
        abspath = sdscat(abspath, "/");
    }

    while (sdslen(relpath) >= 3 &&
            relpath[0] == '.' && relpath[1] == '.' && relpath[2] == '/') {
        // 获取区间内的字符串
        sdsrange(relpath, 3, -1);

        if (sdslen(abspath) > 1) {
            // 绝对路径的倒数第二个字符串开始
            // -2要跳过绝对路径之前添加的"/"
            char *p = abspath + sdslen(abspath) - 2;
            int trimlen = 1;

            // 倒数第二个"/"的位置停止
            while (*p != '/') {
                p--;
                trimlen++;
            }
            sdsrange(abspath, 0, -(trimlen + 1));
        }
    }

    // 组合两部分
    abspath = sdscatsds(abspath, relpath);
    sdsfree(relpath);
    return abspath;
}

// 通过单位转换字符串大小
long long memtoll(const char *p, int *err) {
    const char *u;
    char buf[128];
    long mul;
    long long val;
    unsigned int digits;

    if (err) *err = 0;

    // 查找第一个不是数字的字符串
    u = p;
    if (*u == '-') u++;
    while (*u && isdigit(*u)) u++;

    if (*u == '\0' || !strcasecmp(u, "b")) {
        mul = 1;
    } else if (!strcasecmp(u, "k")) {
        mul = 1000;
    } else if (!strcasecmp(u, "kb")) {
        mul = 1024;
    } else if (!strcasecmp(u, "m")) {
        mul = 1000 * 1000;
    } else if (!strcasecmp(u, "mb")) {
        mul = 1024 * 1024;
    } else if (!strcasecmp(u, "g")) {
        mul = 1000L * 1000 * 1000;
    } else if (!strcasecmp(u, "gb")) {
        mul = 1024L * 1024 * 1024;
    } else {
        if (err) *err = 1;
        mul = 1;
    }

    // 数字长度
    digits = u - p;
    if (digits >= sizeof(buf)) {
        if (err) *err = 1;
        return LLONG_MAX;
    }

    memcpy(buf, p, digits);
    buf[digits] = '\0';
    val = strtoll(buf, NULL, 10);
    return val * mul;
}
