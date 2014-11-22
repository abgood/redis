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
