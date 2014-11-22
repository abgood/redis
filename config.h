#ifndef REDIS_CONFIG_H
#define REDIS_CONFIG_H

void spt_init(int, char **);
void resetServerSaveParams(void);
void loadSererConfig(char *, char *);

#endif
