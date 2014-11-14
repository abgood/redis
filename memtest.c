#include "redis.h"

static struct winsize ws;
size_t progress_printed;
size_t progress_full;

// 内存测试进程开启
void memtest_progress_start(char *title, int pass) {
    int j;
    // 清屏
    printf("\x1b[H\x1b[2J");

    /* fill with dots */
    // -2是因为下面打印了两行数据
    for (j = 0; j < ws.ws_col * (ws.ws_row -2); j++) printf(".");
    printf("Please keep the test running several minutes per GB of memory.\n");
    printf("Also check http://www.memtest86.com/ and http://pyropus.ca/software/memtester/");
    printf("\x1b[H\x1b[2K");
    printf("%s [%d]\n", title, pass);
    progress_printed = 0;
    // 当前屏蔽可打印的总字符数,以窗口大小为准
    // -3是因为打印了3行数据
    progress_full = ws.ws_col * (ws.ws_row - 3);

    fflush(stdout);
}

// 点上打印字符'A'
void memtest_progress_step(size_t curr, size_t size, char c) {
    size_t chars = ((unsigned long long)curr * progress_full) / size, j;

    for (j = 0; j < chars - progress_printed; j++) printf("%c", c);
    progress_printed = chars;
    fflush(stdout);
}

// 分配地址测试
void memtest_addressing(unsigned long *l, size_t bytes) {
    unsigned long words = bytes / sizeof(unsigned long);
    unsigned long j, *p;

    /* Fill */
    p = l;
    for (j = 0; j < words; j++) {
        *p = (unsigned long)p;
        p++;
        if ((j & 0xffff) == 0) memtest_progress_step(j, words * 2, 'A');
    }
}

// test memory
void memtest_test(size_t megabytes, int passes) {
    size_t bytes = megabytes * 1024 * 1024;
    unsigned long *m = malloc(bytes);
    int pass = 0;

    if (!m) {
        fprintf(stderr, "Unable to allocate %zu megabytes: %s", megabytes, strerror(errno));
        exit(1);
    }

    // 做50次循环
    while (pass != passes) {
        pass++;

        // mem test进程开始
        memtest_progress_start("Addressing test", pass);
        // 测试分配的地址
        memtest_addressing(m, bytes);
    }
    free(m);
}

void memtest(size_t megabytes, int passes) {
    // 设置当前终端窗口大小
    if (ioctl(1, TIOCGWINSZ, &ws) == -1) {
        ws.ws_col = 80;
        ws.ws_row = 20;
    }

    // test memory
    memtest_test(megabytes, passes);
    printf("\x1b[H\x1b[2J\n");
    printf("test memory\n");
    printf("\nYour memory passed this test.\n");
    printf("Please if you are still in doubt use the following two tools:\n");
    printf("1) memtest86: http://www.memtest86.com/\n");
    printf("2) memtester: http://pyropus.ca/software/memtester/\n");
    exit(0);
}
