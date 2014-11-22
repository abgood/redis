#ifndef REDIS_H
#define REDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <ctype.h>

#include <sys/time.h>
#include <sys/ioctl.h>

#include "config.h"
#include "zmalloc.h"
#include "dict.h"
#include "sds.h"
#include "util.h"

/* Log levels */
#define REDIS_DEBUG 0   // log debug
#define REDIS_VERBOSE 1   // log verbose
#define REDIS_NOTICE 2  // log notice
#define REDIS_WARNING 3  // log warning
#define REDIS_LOG_RAW (1 << 10)

#define REDIS_MAX_LOGMSG_LEN 1024
#define REDIS_CONFIGLINE_MAX 1024

#define redisPanic(_e) _redisPanic(#_e, __FILE__, __LINE__), _exit(1)

struct saveparam {
    time_t seconds;
    int changes;
};

struct redisServer {
    int verbosity;
    int sentinel_mode;
    int saveparamslen;

    char *logfile;
    char *configfile;

    struct saveparam *saveparams;
};

void _redisPanic(char *, char *, int);
void memtest(size_t, int);

extern struct redisServer server;

#endif
