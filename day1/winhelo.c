//
// Created by marcie on 2020/05/06.
//

void api_end(void);

int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);

char buf[150 * 50];

void HariMain(void) {
    int win = api_openwin(buf, 150, 50, -1, "hello");
    api_end();
}