#include "redis.h"

#define INIT_SETPROCTITLE_REPLACEMENT

struct redisServer server;

// 根据日志等级处理日志
void redisLogRaw(int level, const char *msg) {
    // const int syslogLevelMap[] = {LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING};
    // const char *c = ".-*#";
    // FILE *fp;
    // char buf[64];
    // int rawmode = (level & REDIS_LOG_RAW);
    // int log_to_stdout = server.logfile[0] == '\0';

    level &= 0xff;
    if (level < server.verbosity) return;

    printf("deal with redis raw log");
}

// redis log(组织日志内容)
void redisLog(int level, const char *fmt, ...) {
    va_list ap;
    char msg[REDIS_MAX_LOGMSG_LEN];

    // & 0xff(补码), level永为正
    if ((level & 0xff) < server.verbosity) return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // 根据设定的日志等级来写日志
    redisLogRaw(level, msg);
}

// 内存溢出函数
void redisOutOfMemoryHandler(size_t allocation_size) {
    // redis log函数
    redisLog(REDIS_WARNING, "Out of Memory allocating %zu bytes!", allocation_size);
    // redis 异常
    redisPanic("Redis aborting for OUT OF MEMORY");
}

// 定点模式检测
int checkForSentinelMode(int argc, char **argv) {
    int j;

    if (strstr(argv[0], "redis-sentinel_mode")) return 1;
    for (j = 1; j < argc; j++) {
        if (!strcmp(argv[j], "--sentinel")) return 1;
    }
    return 0;
}

// 初始化redis server配置
void initServerConfig(void) {
    printf("initServerConfig\n");
}

// redis版本
void version(void) {
    printf("Redis server v=2 sha=2:2 malloc=2 bits=64 build=2\n");
    exit(0);
}

void usage(void) {
    fprintf(stderr,"Usage: ./redis-server [/path/to/redis.conf] [options]\n");
    fprintf(stderr,"       ./redis-server - (read config from stdin)\n");
    fprintf(stderr,"       ./redis-server -v or --version\n");
    fprintf(stderr,"       ./redis-server -h or --help\n");
    fprintf(stderr,"       ./redis-server --test-memory <megabytes>\n\n");
    fprintf(stderr,"Examples:\n");
    fprintf(stderr,"       ./redis-server (run the server with default conf)\n");
    fprintf(stderr,"       ./redis-server /etc/redis/6379.conf\n");
    fprintf(stderr,"       ./redis-server --port 7777\n");
    fprintf(stderr,"       ./redis-server --port 7777 --slaveof 127.0.0.1 8888\n");
    fprintf(stderr,"       ./redis-server /etc/myredis.conf --loglevel verbose\n\n");
    fprintf(stderr,"Sentinel mode:\n");
    fprintf(stderr,"       ./redis-server /etc/sentinel.conf --sentinel\n");
    exit(1);
}

int main (int argc, char **argv) {
    struct timeval tv;

    // 初始化相关环境变量与库
#ifdef INIT_SETPROCTITLE_REPLACEMENT
    spt_init(argc, argv);
#endif

    // 设置当前的`locale`命令里的内容
    setlocale(LC_COLLATE, "");

    // 自动分配线程安全
    zmalloc_enable_thread_safeness();

    // 设置自动分配函数句柄(out of memory)
    zmalloc_set_oom_handler(redisOutOfMemoryHandler);

    // 时间异或pid生成随机数种子
    srand(time(NULL) ^ getpid());
    gettimeofday(&tv, NULL);

    // 设置字典hash值种子
    dictSetHashFunctionSeed(tv.tv_sec ^ tv.tv_usec ^ getpid());

    // 定点模式检测
    server.sentinel_mode = checkForSentinelMode(argc, argv);

    // 初始化服务配置
    initServerConfig();

    // 定点模式处理
    if (server.sentinel_mode) {
        printf("deal with sentinel mode\n");
    }

    if (argc >= 2) {
        int j = 1;
        // init empty string
        sds options = sdsempty();
        char *configfile = NULL;

        if (strcmp(argv[1], "-v") == 0 ||
            strcmp(argv[1], "--version") == 0) version();
        if (strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0) usage();
        if (strcmp(argv[1], "--test-memory") == 0) {
            if (argc == 3) {
                // 内存测试还未完全写完
                memtest(atoi(argv[2]), 50);
                exit(0);
            } else {
                fprintf(stderr, "Please specify the amount of memory to test in megabytes.\n");
                fprintf(stderr, "Example: ./redis-server --test-memory 4096\n\n");
                exit(1);
            }
        }

        /*
         * 第一个参数为'--'开头, j始终为1
         * 第一个参数不为'--'开头, j始终为2
         *
         */

        // j为1,第1个参数不是'--'开头的是配置文件
        if (argv[j][0] != '-' || argv[j][1] != '-') {
            configfile = argv[j++];
        }

        // 配置文件参数后还有内容
        while (j != argc) {
            // 所有参数有包括'--'开头的参数
            if (argv[j][0] == '-' && argv[j][1] == '-') {
                if (sdslen(options)) options = sdscat(options, "\n");
                options = sdscat(options, " ");
            } else {
                // append new string, 转义特殊字符, return new pointer
                options = sdscatrepr(options, argv[j], strlen(argv[j]));
                options = sdscat(options, " ");
            }
            j++;
        }

        if (server.sentinel_mode && configfile && *configfile == '-') {
            printf("need redis log\n");
            exit(1);
        }

        if (configfile) {
            server.configfile = getAbsolutePath(configfile);
        }
        resetServerSaveParams();
        loadServerConfig(configfile, options);
    }

    printf("redis done\n");
    return 0;
}
