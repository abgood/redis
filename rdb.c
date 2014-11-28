#include "redis.h"

void rdbRemoveTempFile(pid_t childpid) {
    printf("rdb remove temp file: %d\n", childpid);
}
