#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

/* 他のファイルで作った関数がありますと C compiler に教える */
void io_hlt(void);

void io_cli(void);

void io_out8(int port, int data);

int io_load_eflags(void);

void io_store_eflags(int eflags);

/**
 * パレットを設定する
 */
void init_palette(void);

void set_palette(int start, int end, unsigned char *rgb);

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

void init_screen(char *vram, int xsize, int ysize);

/**
 * 文字を描画する
 * @param vram
 * @param xsize
 * @param x
 * @param y
 * @param c
 * @param font
 */
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);

/**
 * 文字列を描画する
 * @param vram
 * @param xsize
 * @param x
 * @param y
 * @param c
 * @param s
 */
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);


void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "ABC 123");
    putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "Haribote OS");
    putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "Haribote OS");

    while (1) {
        io_hlt();
    }
}

void init_palette(void) {
    static unsigned char table_rgb[16 * 3] = {
            0x00, 0x00, 0x00, // 0: 黒
            0xff, 0x00, 0x00, // 1: 明るい赤
            0x00, 0xff, 0x00, // 2: 明るい緑
            0xff, 0xff, 0x00, // 3: 明るい黃
            0x00, 0x00, 0xff, // 4: 明るい青
            0xff, 0x00, 0xff, // 5: 明るい紫
            0x00, 0xff, 0xff, // 6: 明るい水色
            0xff, 0xff, 0xff, // 7: 白
            0xc6, 0xc6, 0xc6, // 8: 明るい灰色
            0x84, 0x00, 0x00, // 9: 暗い赤
            0x00, 0x84, 0x00, // 9: 暗い緑
            0x84, 0x84, 0x00, // 9: 暗い黄
            0x00, 0x00, 0x84, // 9: 暗い青
            0x84, 0x00, 0x84, // 9: 暗い紫
            0x00, 0x84, 0x84, // 9: 暗い水色
            0x84, 0x84, 0x84, // 9: 暗い灰色
    };
    set_palette(0, 15, table_rgb);
    return;
}

void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags(); // 割り込み許可フラグの値を記録
    io_cli(); // 許可フラグを0にして割り込みを禁止する
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags); // 割り込み許可フラグを元に戻す
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
    return;
}

void init_screen(char *vram, int xsize, int ysize) {
    boxfill8(vram, xsize, COL8_008484, 0, 0, xsize - 1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 28, xsize - 1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF, 0, ysize - 27, xsize - 1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 26, xsize - 1, ysize - 1);

    boxfill8(vram, xsize, COL8_FFFFFF, 3, ysize - 24, 59, ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 24, 2, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 3, ysize - 4, 59, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize - 5);
    boxfill8(vram, xsize, COL8_000000, 2, ysize - 3, 59, ysize - 3);
    boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 3, ysize - 3);

    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize - 4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize - 4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize - 3, xsize - 4, ysize - 3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3, ysize - 24, xsize - 3, ysize - 3);

    return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
    int i, j, mask;
    char *p, d;
    for (i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        for (j = 0; j < 8; j++) {
            mask = 0x80 >> j;
            if ((d & mask) != 0) {
                p[j] = c;
            }
        }
    }
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
}