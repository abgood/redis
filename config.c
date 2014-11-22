#include "redis.h"

// 重置保存的参数
void resetServerSaveParams(void) {
    zfree(server.saveparams);
    server.saveparams = NULL;
    server.saveparamslen = 0;
}

// 加载server配置字符串
void loadServerConfigFromString(char *config) {
    char *err = NULL;
    int linenum = 0, totlines, i;
    sds *lines;

    lines = sdssplitlen(config, strlen(config), "\n", 1, &totlines);

    // loop each line
    for (i = 0; i < totlines; i++) {
        sds *argv;
        int argc;

        linenum = i + 1;
        // 去除字符串
        lines[i] = sdstrim(lines[i], " \t\r\n");

        // 跳过注释行和空行
        if (lines[i][0] == '#' || lines[i][0] == '\0') continue;

        // 分隔参数
        argv = sdssplitargs(lines[i], &argc);
    }
}

// 加载server配置信息, file and options
void loadServerConfig(char *filename, char *options) {
    sds config = sdsempty();
    char buf[REDIS_CONFIGLINE_MAX + 1];

    if (filename) {
        FILE *fp;

        // "-"表示标准输入
        if (filename[0] == '-' && filename[1] == '\0') {
            fp = stdin;
        } else {
            if ((fp = fopen(filename, "r")) == NULL) {
                printf("can't open config file\n");
                exit(1);
            }
        }

        // 读取文件内容保存至buf
        while (fgets(buf, REDIS_CONFIGLINE_MAX + 1, fp) != NULL) {
            config = sdscat(config, buf);
        }
        if (fp != stdin) fclose(fp);
    }

    // 追加附加条件
    if (options) {
        config = sdscat(config, "\n");
        config = sdscat(config, options);
    }

    loadServerConfigFromString(config);
    sdsfree(config);
}
