//
// Created by marcie on 2020/05/06.
//
void api_end(void);

int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);

void api_putstrwin(int win, int x, int y, int col, int len, char *str);

void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);

void api_initmalloc(void);

char *api_malloc(int size);

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(150 * 50);
    int win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfilwin(win, 8, 36, 141, 43, 3 /* 黄 */);
    api_putstrwin(win, 28, 28, 0 /* 黒 */, 12, "hello, world");
    api_end();
}