#ifndef REDIS_UTIL_H
#define REDIS_UTIL_H

sds getAbsolutePath(char *);
long long memtoll(const char *, int *);
int ll2string(char *, size_t, long long);

#endif
