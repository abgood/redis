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
#include <limits.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/epoll.h>

#include "config.h"
#include "zmalloc.h"
#include "dict.h"
#include "sds.h"
#include "util.h"
#include "adlist.h"

/* Log levels */
#define REDIS_DEBUG 0   // log debug
#define REDIS_VERBOSE 1   // log verbose
#define REDIS_NOTICE 2  // log notice
#define REDIS_WARNING 3  // log warning
#define REDIS_LOG_RAW (1 << 10)

#define REDIS_BINDADDR_MAX 16

#define REDIS_MAX_LOGMSG_LEN 1024
#define REDIS_CONFIGLINE_MAX 1024

/* Redis maxmemory strategies */
#define REDIS_MAXMEMORY_VOLATILE_LRU 0
#define REDIS_MAXMEMORY_VOLATILE_TTL 1
#define REDIS_MAXMEMORY_VOLATILE_RANDOM 2
#define REDIS_MAXMEMORY_ALLKEYS_LRU 3
#define REDIS_MAXMEMORY_ALLKEYS_RANDOM 4
#define REDIS_MAXMEMORY_NO_EVICTION 5
#define REDIS_DEFAULT_MAXMEMORY_POLICY REDIS_MAXMEMORY_VOLATILE_LRU

/* Slave replication state - from the point of view of the slave. */
#define REDIS_REPL_NONE 0
#define REDIS_REPL_CONNECT 1
#define REDIS_REPL_CONNECTING 2
#define REDIS_REPL_RECEIVE_PONG 3
#define REDIS_REPL_TRANSFER 4
#define REDIS_REPL_CONNECTED 5

#define REDIS_REPL_BACKLOG_MIN_SIZE (16 * 2014)

#define redisPanic(_e) _redisPanic(#_e, __FILE__, __LINE__), _exit(1)

#define REDIS_LRU_BITS 24
#define REDIS_LRU_CLOCK_MAX ((1<<REDIS_LRU_BITS)-1)
#define REDIS_LRU_CLOCK_RESOLUTION 1

#define REDIS_SHARED_SELECT_CMDS 10
#define REDIS_SHARED_INTEGERS 10000
#define REDIS_SHARED_BULKHDR_LEN 32
#define REDIS_MAX_LOGMSG_LEN    1024

#define REDIS_ENCODING_RAW 0
#define REDIS_ENCODING_INT 1
#define REDIS_ENCODING_HT 2
#define REDIS_ENCODING_ZIPMAP 3
#define REDIS_ENCODING_LINKEDLIST 4
#define REDIS_ENCODING_ZIPLIST 5
#define REDIS_ENCODING_INTSET 6
#define REDIS_ENCODING_SKIPLIST 7

#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2
#define REDIS_ZSET 3
#define REDIS_HASH 4

#define REDIS_MIN_RESERVED_FDS 32
#define REDIS_EVENTLOOP_FDSET_INCR (REDIS_MIN_RESERVED_FDS + 96)

struct saveparam {
    time_t seconds;
    int changes;
};

struct redisServer {
    int verbosity;
    int sentinel_mode;
    int saveparamslen;
    int maxidletime;
    int tcpkeepalive;
    int port;
    int tcp_backlog;
    int bindaddr_count;
    int syslog_enabled;
    int syslog_facility;
    int dbnum;
    int maxmemory_policy;
    int maxmemory_samples;
    int masterport;
    int repl_state;
    int repl_ping_slave_period;
    int repl_timeout;
    int repl_disable_tcp_nodelay;
    int daemonize;
    int slaveseldb;

    char *logfile;
    char *configfile;
    char *bindaddr[REDIS_BINDADDR_MAX];
    char *unixsocket;
    char *syslog_ident;
    char *masterhost;
    char *repl_backlog;
    char *masterauth;

    mode_t unixsocketperm;
    unsigned int maxclients;
    unsigned lruclock:REDIS_LRU_BITS;
    unsigned long long maxmemory;
    time_t repl_backlog_time_limit;

    list *clients;
    list *clients_to_close;
    list *slaves, *monitors;
    list *unblocked_clients;
    list *ready_keys;

    long long repl_backlog_size;
    long long repl_backlog_histlen;
    long long repl_backlog_idx;
    long long repl_backlog_off;
    long long master_repl_offset;

    struct saveparam *saveparams;
};

void _redisPanic(char *, char *, int);
void memtest(size_t, int);

extern struct redisServer server;
void resizeReplicationBacklog(long long);

#endif
