//
// Created by marcie on 2020/05/10.
//
void api_end(void);

int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);

void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);

void api_initmalloc(void);

char *api_malloc(int size);

void api_point(int win, int x, int y, int col);

void api_refreshwin(int win, int x0, int y0, int x1, int y1);

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(150 * 100);
    int win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfilwin(win + 1, 6, 26, 143, 93, 0 /*黒 */);
    api_point(win + 1, 75, 59, 3 /*黄*/ );
    api_refreshwin(win, 6, 26, 143, 93);
    api_end();
}