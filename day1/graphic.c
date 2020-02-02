#include "bootpack.h"

/**
 * パレットを設定する
 */
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


/**
 * 文字を描画する
 * @param vram
 * @param xsize
 * @param x
 * @param y
 * @param c
 * @param font
 */
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

/**
 * 文字列を描画する
 * @param vram
 * @param xsize
 * @param x
 * @param y
 * @param c
 * @param s
 */
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
}


void init_mouse_cursor8(char *mouse, char bc) {
    static char cursor[16][16] = {
            "**************..",
            "*OOOOOOOOOOO*...",
            "*OOOOOOOOOO*....",
            "*OOOOOOOOO*.....",
            "*OOOOOOOO*......",
            "*OOOOOOO*.......",
            "*OOOOOOO*.......",
            "*OOOOOOOO*......",
            "*OOOO**OOO*.....",
            "*OOO*..*OOO*....",
            "*OO*....*OOO*...",
            "*O*......*OOO*..",
            "**........*OOO*.",
            "*..........*OOO*",
            "............*OO*",
            ".............***"
    };
    int x, y;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            switch (cursor[x][y]) {
                case '*':
                    mouse[y * 16 + x] = COL8_000000;
                    break;
                case 'O':
                    mouse[y * 16 + x] = COL8_FFFFFF;
                    break;
                default:
                    mouse[y * 16 + x] = bc;
            }
        }
    }
    return;
}

void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize) {
    int x, y;
    for (y = 0; y < pysize; y++) {
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}

