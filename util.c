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

// 以十进制返回字符串长度
uint32_t digits10(uint64_t v) {
    if (v < 10) return 1;
    if (v < 100) return 2;
    if (v < 1000) return 3;
    if (v < 1000000000000UL) {
        if (v < 100000000UL) {
            if (v < 1000000) {
                if (v < 10000) return 4;
                return 5 + (v >= 100000);
            }
            return 7 + (v >= 10000000UL);
        }
        if (v < 10000000000UL) {
            return 9 + (v >= 1000000000UL);
        }
        return 11 + (v >= 100000000000UL);
    }
    return 12 + digits10(v / 1000000000000UL);
}

// 把长变量转换进字符串里
int ll2string(char *dst, size_t dstlen, long long svalue) {
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int negative;
    unsigned long long value;

    // 负数则转换, 并标记
    if (svalue < 0) {
        if (svalue != LLONG_MIN) {
            value = -svalue;
        } else {
            value = ((unsigned long long)LLONG_MAX) + 1;
        }
        negative = 1;
    } else {
        value = svalue;
        negative = 0;
    }

    // 检测长度
    uint32_t const length = digits10(value) + negative;
    if (length >= dstlen) return 0;

    // 空白符
    uint32_t next = length;
    dst[next] = '\0';
    next--;

    while (value >= 100) {
        int const i = (value % 100) * 2;
        // 大于100的除等于
        value /= 100;
        // 第3位
        dst[next] = digits[i + 1];
        // 第2位
        dst[next - 1] = digits[i];
        next -= 2;
    }

    if (value < 10) {
        // 大于100的和小于10的
        dst[next] = '0' + (uint32_t)value;
    } else {
        // printf("%d\n", value);
        int i = (uint32_t)value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    if (negative) dst[0] = '-';
    return length;
}
