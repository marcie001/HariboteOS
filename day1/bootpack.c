/* 他のファイルで作った関数がありますと C compiler に教える */
void io_hlt(void);

void HariMain(void) {
    int i;
    char *p;

    p = (char *) 0xa0000;
    for (i = 0; i <= 0xffff; i++) {
        // メモリ p + i 番地に i & 0x0f を書き込む
        i[p] = i & 0x0f;
        // p[i] や i[p] は *(p + i) と完全に同じ意味

        // 78ページ Column-3, 82ページ Column-4 に詳しい解説がある
    }

    while (1) {
        io_hlt();
    }
}
