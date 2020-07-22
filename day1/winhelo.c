//
// Created by marcie on 2020/05/06.
//
#include "libapi.h"

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(150 * 50);
    int win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfilwin(win, 8, 36, 141, 43, 3 /* 黄 */);
    api_putstrwin(win, 28, 28, 0 /* 黒 */, 12, "hello, world");
    api_end();
}