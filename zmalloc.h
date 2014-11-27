#ifndef REDIS_ZMALLOC_H
#define REDIS_ZMALLOC_H

void zmalloc_enable_thread_safeness(void);
void zmalloc_set_oom_handler(void (*)(size_t));
void *zmalloc(size_t);
void *zrealloc(void *, size_t);
void zfree(void *);
char *zstrdup(const char *);

#endif
