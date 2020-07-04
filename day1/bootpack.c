#include "bootpack.h"

#define KEYCMD_LED  0xed

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x);

int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c);

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
    *((int *) 0xfe4) = (int) shtctl;
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
    sheet_slide(sht_win, 300, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    struct CONSOLE *cons;
    struct FIFO32 keycmd;
    int keycmd_buf[32];
    int i, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    int key_ctrl = 0, key_alt = 0;
    int j, x, y, mmx = -1, mmy = -1;
    struct SHEET *sht = 0, *key_win = sht_win;
    sht_cons->task = task_cons;
    sht_cons->flags |= 0x20; // カーソルあり
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
            if (key_win->flags == 0) {
                // 入力ウィンドウが閉じられた
                key_win = shtctl->sheets[shtctl->top - 1];
                cursor_c = keywin_on(key_win, sht_win, cursor_c);
            }
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
                if (i == 256 + 0x2e && key_ctrl != 0 && task_cons->tss.ss0 != 0) {
                    // Ctrl + c
                    cons = (struct CONSOLE *) *((int *) 0x0fec);
                    cons_putstr0(cons, "\nBreak(key) :\n");
                    io_cli(); // レジスタ変更中にタスクが変わると困るので
                    task_cons->tss.eax = (int) &(task_cons->tss.esp0);
                    task_cons->tss.eip = (int) asm_end_app;
                    io_sti();
                    continue;
                }
                if (s[0] != 0) {
                    // 通常文字
                    if (key_win == sht_win) {
                        // タスク A へ
                        if (cursor_x < 128) {
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {
                        // コンソールへ
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }
                if (i == 256 + 0x0e) {
                    // バックスペース
                    if (key_win == sht_win) {
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
                    if (key_alt != 0 && shtctl->top > 2) {
                        sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                    } else {
                        cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
                        j = key_win->height - 1;
                        if (j == 0) {
                            j = shtctl->top - 1;
                        }
                        key_win = shtctl->sheets[j];
                        cursor_c = keywin_on(key_win, sht_win, cursor_c);
                    }
                }
                if (i == 256 + 0x1c) {
                    // Enter
                    if (key_win != sht_win) {
                        fifo32_put(&task_cons->fifo, 10 + 256);
                    }
                }
                if (i == 256 + 0x1d) {
                    // 左 Ctrl ON
                    key_ctrl |= 1;
                }
                if (i == 256 + 0x2a) {
                    // 左シフトON
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {
                    // 右シフトON
                    key_shift |= 2;
                }
                if (i == 256 + 0x38) {
                    // 左 Alt ON
                    key_alt |= 1;
                }
                if (i == 256 + 0x9d) {
                    // 左 Ctrl OFF
                    key_ctrl &= ~1;
                }
                if (i == 256 + 0xaa) {
                    // 左シフトOFF
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) {
                    // 右シフトOFF
                    key_shift &= ~2;
                }
                if (i == 256 + 0xb8) {
                    // 左 Alt OFF
                    key_alt &= ~1;
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
                        // 左ボタン
                        if (mmx < 0) {
                            // 通常モードの場合、上の下敷きから順番にマウスが指している下敷きを探す
                            for (j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top - 1);
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx;
                                            mmy = my;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            // 「❌」ボタンクリック
                                            if ((sht->flags & 0x10) != 0) {
                                                // アプリが作ったウィンドウの場合
                                                cons = (struct CONSOLE *) *((int *) 0x0fec);
                                                cons_putstr0(cons, "\nBreak(mouse) :\n");
                                                io_cli(); // 強制終了痛にタスクが変わると困るので
                                                task_cons->tss.eax = (int) &(task_cons->tss.esp0);
                                                task_cons->tss.eip = (int) asm_end_app;
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            // ウィンドウ移動モードの場合
                            x = mx - mmx; // マウスの移動量を計算
                            y = my - mmy;
                            sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
                            mmx = mx; // 移動後の座標に更新
                            mmy = my;
                        }
                    } else {
                        // 左ボタンを押していない
                        mmx = -1;
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

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x) {
    change_wtitle8(key_win, 0);
    if (key_win == sht_win) {
        cur_c = -1;
        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
    } else {
        if ((key_win->flags & 0x20) != 0) {
            // コンソールのカーソル OFF
            fifo32_put(&key_win->task->fifo, 3);
        }
    }
    return cur_c;
}

int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c) {
    change_wtitle8(key_win, 1);
    if (key_win == sht_win) {
        cur_c = COL8_000000;
    } else {
        if ((key_win->flags & 0x20) != 0) {
            // コンソールのカーソル ON
            fifo32_put(&key_win->task->fifo, 2);
        }
    }
    return cur_c;
}
