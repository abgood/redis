#include "redis.h"

/* 重新拷贝相关变量在赋值是为了让相关变量内容存在于程序的上下文之间
 * 而不用关心linux程序在虚拟内存里面的分布 */

extern char *program_invocation_name;
extern char *program_invocation_short_name;
extern char **environ;

static struct {
    const char *arg0;   // 程序名
    char *base, *end;   // 命令行参数与环境变量的开始与结尾
    char *nul;          // 程序名末尾的后一位上
    _Bool reset;
    int error;
} SPT;

static int spt_clearenv(void) {
    clearenv();
    return 0;
}

// oldenv为当前环境变量地址
static int spt_copyenv(char **oldenv) {
    char *eq;
    int i, error;

    // oldenv是envp, envp等于environ
    if (environ != oldenv) {
        return 0;
    }

    // 清空环境变量
    if ((error = spt_clearenv())) {
        goto error;
    }

    // 重设环境变量, 使之本程序有效
    for (i = 0; oldenv[i]; i++) {
        if (!(eq = strchr(oldenv[i], '='))) {
            continue;
        }

        *eq = '\0';
        error = (0 != setenv(oldenv[i], eq + 1, 1)) ? errno : 0;
        *eq = '=';

        if (error) {
            goto error;
        }
    }

    return 0;

error:
    environ = oldenv;
    return error;
}

static int spt_copyargs(int argc, char **argv) {
    char *tmp;
    int i;

    for (i = 1; i < argc || (i >= argc && argv[i]); i++) {
        if (!argv[i]) {
            continue;
        }

        if (!(tmp = strdup(argv[i]))) {
            return errno;
        }

        argv[i] = tmp;
    }

    return 0;
}

void spt_init(int argc, char **argv) {
    // environ是一个指针数组,指向linux "environment"
    char **envp = environ;
    char *base, *end, *nul, *tmp;
    int i, error;

    // base的值为程序名
    if (!(base = argv[0])) {
        return;
    }

    // a + 1 == a + sizeof(a[0])
    // &a + 1 == a + sizeof(a) == int (*a)[]
    // base is point, &base is point to point
    nul = &base[strlen(base)];
    // 程序名后的第一个地址
    end = nul + 1;

    // end的值保持在命令行参数末尾的后一位上
    for (i = 0; i < argc || (i >= argc && argv[i]); i++) {
        if (!argv[i] || argv[i] < end) {
            continue;
        }

        end = argv[i] + strlen(argv[i]) + 1;
    }

    // end的值保持在环境变量末尾的后一位上
    for (i = 0; envp[i]; i++) {
        if (envp[i] < end) {
            continue;
        }

        end = envp[i] + strlen(envp[i]) + 1;
    }

    // argv[0] save to SPT struct
    if (!(SPT.arg0 = strdup(argv[0]))) {
        goto syerr;
    }

    // 带路径的程序名
    if (!(tmp = strdup(program_invocation_name))) {
        goto syerr;
    }
    program_invocation_name = tmp;

    // 不带路径的程序名
    if (!(tmp = strdup(program_invocation_short_name))) {
        goto syerr;
    }
    program_invocation_short_name = tmp;

    // printf("%s, %s\n", program_invocation_name, program_invocation_short_name);

    // 拷贝环境变量
    if ((error = spt_copyenv(envp))) {
        goto syerr;
    }

    // 拷贝命令行参数
    if ((error = spt_copyargs(argc, argv))) {
        goto error;
    }

    // 程序名后一位为空格
    SPT.nul = nul;
    // 程序名
    SPT.base = base;
    // 命令行参数与环境变量长度
    SPT.end = end;

    return;

syerr:
    error = errno;
error:
    SPT.error = error;
}
