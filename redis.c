#include "redis.h"

#define INIT_SETPROCTITLE_REPLACEMENT

struct redisServer server;
struct sharedObjectsStruct shared;

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

// 后台模式
void daemonize(void) {
    int fd;

    if (fork() != 0) {
        exit(0);
    }
    setsid();

    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
}

// shutdown处理函数
static void sigShutdownHandler(int sig) {
    char *msg;

    switch (sig) {
    case SIGINT:
        msg = "Received SIGINT scheduling shutdown...";
        break;
    case SIGTERM:
        msg = "Received SIGTERM scheduling shutdown...";
        break;
    default:
        msg = "Received shutdown signal, scheduling shutdown...";
    }

    if (server.shutdown_asap && sig == SIGINT) {
        printf("You insist... exiting now.");
        rdbRemoveTempFile(getpid());
        exit(1);
    } else if (server.loading) {
        exit(0);
    }

    printf("%s\n", msg);

    // redisLogFromHandler(REDIS_WARNING, msg);
    server.shutdown_asap = 1;
}

// set signal handler
void setupSignalHandlers(void) {
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigShutdownHandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

#ifdef HAVE_BACKTRACE
    printf("have backtrace\n");
#endif

    return;
}

// 创建共享对象
void createSharedObjects(void) {
    int j;

    shared.crlf = createObject(REDIS_STRING, sdsnew("\r\n"));
    shared.ok = createObject(REDIS_STRING,sdsnew("+OK\r\n"));
    shared.err = createObject(REDIS_STRING,sdsnew("-ERR\r\n"));
    shared.emptybulk = createObject(REDIS_STRING,sdsnew("$0\r\n\r\n"));
    shared.czero = createObject(REDIS_STRING,sdsnew(":0\r\n"));
    shared.cone = createObject(REDIS_STRING,sdsnew(":1\r\n"));
    shared.cnegone = createObject(REDIS_STRING,sdsnew(":-1\r\n"));
    shared.nullbulk = createObject(REDIS_STRING,sdsnew("$-1\r\n"));
    shared.nullmultibulk = createObject(REDIS_STRING,sdsnew("*-1\r\n"));
    shared.emptymultibulk = createObject(REDIS_STRING,sdsnew("*0\r\n"));
    shared.pong = createObject(REDIS_STRING,sdsnew("+PONG\r\n"));
    shared.queued = createObject(REDIS_STRING,sdsnew("+QUEUED\r\n"));
    shared.emptyscan = createObject(REDIS_STRING,sdsnew("*2\r\n$1\r\n0\r\n*0\r\n"));
    shared.wrongtypeerr = createObject(REDIS_STRING,sdsnew(
        "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"));
    shared.nokeyerr = createObject(REDIS_STRING,sdsnew(
        "-ERR no such key\r\n"));
    shared.syntaxerr = createObject(REDIS_STRING,sdsnew(
        "-ERR syntax error\r\n"));
    shared.sameobjecterr = createObject(REDIS_STRING,sdsnew(
        "-ERR source and destination objects are the same\r\n"));
    shared.outofrangeerr = createObject(REDIS_STRING,sdsnew(
        "-ERR index out of range\r\n"));
    shared.noscripterr = createObject(REDIS_STRING,sdsnew(
        "-NOSCRIPT No matching script. Please use EVAL.\r\n"));
    shared.loadingerr = createObject(REDIS_STRING,sdsnew(
        "-LOADING Redis is loading the dataset in memory\r\n"));
    shared.slowscripterr = createObject(REDIS_STRING,sdsnew(
        "-BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
    shared.masterdownerr = createObject(REDIS_STRING,sdsnew(
        "-MASTERDOWN Link with MASTER is down and slave-serve-stale-data is set to 'no'.\r\n"));
    shared.bgsaveerr = createObject(REDIS_STRING,sdsnew(
        "-MISCONF Redis is configured to save RDB snapshots, but is currently not able to persist on disk. Commands that may modify the data set are disabled. Please check Redis logs for details about the error.\r\n"));
    shared.roslaveerr = createObject(REDIS_STRING,sdsnew(
        "-READONLY You can't write against a read only slave.\r\n"));
    shared.noautherr = createObject(REDIS_STRING,sdsnew(
        "-NOAUTH Authentication required.\r\n"));
    shared.oomerr = createObject(REDIS_STRING,sdsnew(
        "-OOM command not allowed when used memory > 'maxmemory'.\r\n"));
    shared.execaborterr = createObject(REDIS_STRING,sdsnew(
        "-EXECABORT Transaction discarded because of previous errors.\r\n"));
    shared.noreplicaserr = createObject(REDIS_STRING,sdsnew(
        "-NOREPLICAS Not enough good slaves to write.\r\n"));
    shared.space = createObject(REDIS_STRING,sdsnew(" "));
    shared.colon = createObject(REDIS_STRING,sdsnew(":"));
    shared.plus = createObject(REDIS_STRING,sdsnew("+"));

    for (j = 0; j < REDIS_SHARED_SELECT_CMDS; j++) {
        char dictid_str[64];
        int dictid_len;

        // 把长变量转换进字符串里
        dictid_len = ll2string(dictid_str, sizeof(dictid_str), j);
        shared.select[j] = createObject(REDIS_STRING,
            sdscatprintf(sdsempty(),
                "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                dictid_len, dictid_str));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);
    shared.del = createStringObject("DEL",3);
    shared.rpop = createStringObject("RPOP",4);
    shared.lpop = createStringObject("LPOP",4);
    shared.lpush = createStringObject("LPUSH",5);

    for (j = 0; j < REDIS_SHARED_INTEGERS; j++) {
        shared.integers[j] = createObject(REDIS_STRING, (void *)(long)j);
        shared.integers[j]->encoding = REDIS_ENCODING_INT;
    }

    for (j = 0; j < REDIS_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] = createObject(REDIS_STRING,
            sdscatprintf(sdsempty(),"*%d\r\n",j));
        shared.bulkhdr[j] = createObject(REDIS_STRING,
            sdscatprintf(sdsempty(),"$%d\r\n",j));
    }

    shared.minstring = createStringObject("minstring",9);
    shared.maxstring = createStringObject("maxstring",9);
}

// 打开文件符限制
void adjustOpenFilesLimit(void) {
    rlim_t maxfiles = server.maxclients + REDIS_MIN_RESERVED_FDS;
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        
    }
}

// init server
void initServer(void) {
    int j;

    // ignore sighup, sigpipe
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    // 设置信号处理函数
    setupSignalHandlers();

    if (server.syslog_enabled) {
        openlog(server.syslog_ident, LOG_PID | LOG_NDELAY | LOG_NOWAIT,
                server.syslog_facility);
    }

    // 当前客户端相关状态
    server.current_client = NULL;

    // 创建一个链表, 用于存储客户端
    server.clients = listCreate();

    // 创建客户端关闭链表
    server.clients_to_close = listCreate();

    // 从redis链表
    server.slaves = listCreate();

    // 监控链表
    server.monitors = listCreate();
    server.slaveseldb = -1;

    // 未锁定客户端链表
    server.unblocked_clients = listCreate();

    // 准备key链表
    server.ready_keys = listCreate();

    // 创建共享对象
    createSharedObjects();

    // 打开文件描述符限制
    adjustOpenFilesLimit();
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
        sdsfree(options);
    } else {
        printf("warning: no config file specified\n");
    }

    // 后台模式
    if (server.daemonize) {
        daemonize();
    }

    // init server
    initServer();

    printf("redis done\n");
    return 0;
}
