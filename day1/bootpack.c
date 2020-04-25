#include "bootpack.h"

#define KEYCMD_LED  0xed

struct FILEINFO {
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

void console_task(struct SHEET *sheet, unsigned int memtotal);

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

int cons_newline(int y, struct SHEET *sheet);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    unsigned char buf_mouse[256];
    // http://oswiki.osask.jp/?(AT)keyboard
    // @formatter:off
    static char keytable0[0x80] = {
            0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
            'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
            'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
            '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '\\',0,   0,
    };
    static char keytable1[0x80] = {
            0,   0,   '!', '"','#', '$', '%', '&', '\'','(', ')', '~', '=', '~', 0,   0,
            'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
            'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
            '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0,
    };
    // @formatter:on

    // ハードウェアがいろいろなデータを溜め込んで不具合を起こさないうちに早く割り込みを受け付けられるように
    // まずGDT/IDTを作り直し、PICを初期化してio_stiを呼び出す
    init_gdtidt();
    init_pic();
    io_sti();

    fifo32_init(&fifo, 128, fifobuf, 0);
    init_pit();
    io_out8(PIC0_IMR, 0xf8); // PITとPIC1とキーボードを許可（11111000)
    io_out8(PIC1_IMR, 0xef); // マウスを許可(11101111)

    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    struct TASK *task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    // sht_back
    struct SHEET *sht_back = sheet_alloc(shtctl);
    unsigned char *buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); // 透明色なし
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    // sht_cons
    struct SHEET *sht_cons = sheet_alloc(shtctl);
    unsigned char *buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); // 透明色なし
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    struct TASK *task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task_cons->tss.eip = (int) &console_task;
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
    *((int *) (task_cons->tss.esp + 8)) = memtotal;
    task_run(task_cons, 2, 2);

    // sht_win
    struct SHEET *sht_win = sheet_alloc(shtctl);
    unsigned char *buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 144, 52, -1); // 透明色なし
    make_window8(buf_win, 144, 52, "task_a", 1);
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
    int cursor_x = 8;
    int cursor_c = COL8_FFFFFF;
    struct TIMER *timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    // sht_mouse
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
    int my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons, 32, 4);
    sheet_slide(sht_win, 8, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    struct FIFO32 keycmd;
    int keycmd_buf[32];
    int i, key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);
    while (1) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            task_sleep(task_a);
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {
                // キーボードデータ
                if (i < 256 + 0x80) {
                    // キーコードを文字コードに変換
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
                if ('A' <= s[0] && s[0] <= 'Z') {
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20; // 大文字を小文字に変換
                    }
                }
                if (s[0] != 0) {
                    // 通常文字
                    if (key_to == 0) {
                        if (cursor_x < 128) {
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }
                if (i == 256 + 0x0e) {
                    // バックスペース
                    if (key_to == 0) {
                        if (cursor_x > 8) {
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                            cursor_x -= 8;
                        }
                    } else {
                        fifo32_put(&task_cons->fifo, 8 + 256);
                    }
                }
                if (i == 256 + 0x0f) {
                    // Tab
                    if (key_to == 0) {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                        // カーソル非表示
                        cursor_c = -1;
                        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
                        // コンソールのカーソルON
                        fifo32_put(&task_cons->fifo, 2);
                    } else {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                        // カーソル表示
                        cursor_c = COL8_000000;
                        // コンソールのカーソルOFF
                        fifo32_put(&task_cons->fifo, 3);
                    }
                    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }
                if (i == 256 + 0x1c) {
                    // Enter
                    if (key_to != 0) {
                        fifo32_put(&task_cons->fifo, 10 + 256);
                    }
                }
                if (i == 256 + 0x2a) {
                    // 左シフトON
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {
                    // 右シフトON
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) {
                    // 左シフトOFF
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) {
                    // 左シフトOFF
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) {
                    // CapsLock
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) {
                    // NumLock
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) {
                    // ScrollLock
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0xfa) {
                    // キーボードが無事データを受け取った
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) {
                    // キーボードがデータを受け取れなかった
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
                // カーソル再表示
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                }
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if (512 <= i && i <= 767) {
                // マウスデータ
                if (mouse_decode(&mdec, i - 512) != 0) {
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    sheet_slide(sht_mouse, mx, my);

                    if ((mdec.btn & 0x01) != 0) {
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                }
            } else if (i <= 1) {
                // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &fifo, 0); // 次はデータを0とする
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                } else {
                    timer_init(timer, &fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                }
                timer_settime(timer, 50);
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
                }
            }
        }
    }
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act) {
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act) {
    static char closebtn[14][16] = {
            "OOOOOOOOOOOOOOO@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQQQ@@QQQQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "O$$$$$$$$$$$$$$@",
            "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c, tc, tbc;
    if (act != 0) {
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    } else {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts8_asc(buf, xsize, 24, 4, tc, title);
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            switch (c) {
                case '@':
                    c = COL8_000000;
                    break;
                case '$':
                    c = COL8_848484;
                    break;
                case 'Q':
                    c = COL8_C6C6C6;
                    break;
                default:
                    c = COL8_FFFFFF;
                    break;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

/**
 * 文字を描画する
 * @param sht シート
 * @param x 表示位置x座標
 * @param y 表示位置y座標
 * @param c 文字色
 * @param b 背景色
 * @param s 表示する文字列
 * @param l 表示する文字列の長さ
 */
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l) {
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
    return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c) {
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TASK *task = task_now();
    int fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;

    fifo32_init(&task->fifo, 128, fifobuf, task);

    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // プロンプト表示
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    char s[30], cmdline[30];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i, x, y;
    while (1) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) {
                // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {
                cursor_c = COL8_FFFFFF;
            }
            if (i == 3) {
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, 28, cursor_x + 7, 43);
                cursor_c = -1;
            }
            if (256 <= i && i <= 511) {
                // キーボードデータ
                if (i == 8 + 256) {
                    // バックスペース
                    if (cursor_x > 16) {
                        // カーソルをスペースで消してから、カーソルを1つ戻す
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    // Enter
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor_x / 8 - 2] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    if (mystrcmp(cmdline, "mem") == 0) {
                        // mem コマンド
                        mysprintf(s, "total %dMB", memtotal / (1024 * 1024));
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        mysprintf(s, "free  %dKB", memman_total(memman) / 1024);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (mystrcmp(cmdline, "cls") == 0) {
                        // cls コマンド
                        for (y = 28; y < 28 + 128; ++y) {
                            for (x = 8; x < 8 + 240; ++x) {
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    } else if (mystrcmp(cmdline, "dir") == 0) {
                        // dir コマンド
                        for (x = 0; x < 224; ++x) {
                            if (finfo[x].name[0] == 0x00) {
                                // ファイル名の1文字目が0の場合、それ以上ファイルはない、という意味
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                // ファイル名の1文字目が0xe5の場合、そのファイルは削除済み、という意味
                                if ((finfo[x].type & 0x18) == 0) {
                                    mysprintf(s, "filename.ext %d", finfo[x].size);
                                    for (y = 0; y < 8; ++y) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (cmdline[0] != 0) {
                        // 存在しないコマンドの実行
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    // プロンプト表示
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor_x = 16;
                } else {
                    // 一般文字
                    if (cursor_x < 240) {
                        // 1文字表示してから、カーソルを1つ進める
                        s[0] = i - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = i - 256;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            // カーソル再表示
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
    // return は書かない。 return は呼び出し元への JMP 命令のようなものだから。この関数は呼び出し元がないので。
    // ちなみに、return 先の番地は [ESP] にある
}

/**
 * コンソールの改行処理を行う
 * @param cursor_y カーソルのy座標
 * @param sheet シート
 * @return カーソルのy座標
 */
int cons_newline(int cursor_y, struct SHEET *sheet) {
    int x, y;
    if (cursor_y < 28 + 112) {
        cursor_y += 16;
    } else {
        // スクロール
        for (y = 28; y < 28 + 112; ++y) {
            for (x = 8; x < 8 + 240; ++x) {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; ++y) {
            for (x = 8; x < 8 + 240; ++x) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    return cursor_y;
}
