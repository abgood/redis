#include "redis.h"

// 重置replication backlog大小
void resizeReplicationBacklog(long long newsize) {
    if (newsize < REDIS_REPL_BACKLOG_MIN_SIZE) {
        newsize = REDIS_REPL_BACKLOG_MIN_SIZE;
    }
    if (server.repl_backlog_size == newsize) return;

    server.repl_backlog_size = newsize;
    if (server.repl_backlog != NULL) {
        // free old buffer, realloc new space
        zfree(server.repl_backlog);
        server.repl_backlog = zmalloc(server.repl_backlog_size);
        server.repl_backlog_histlen = 0;
        server.repl_backlog_idx = 0;
        server.repl_backlog_off = server.master_repl_offset + 1;
    }
}
