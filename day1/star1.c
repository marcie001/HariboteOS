//
// Created by marcie on 2020/05/10.
//
#include "libapi.h"

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(150 * 100);
    int win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfilwin(win + 1, 6, 26, 143, 93, 0 /*黒 */);
    api_point(win + 1, 75, 59, 3 /*黄*/ );
    api_refreshwin(win, 6, 26, 143, 93);
    api_end();
}