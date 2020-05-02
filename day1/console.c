//
// Created by marcie on 2020/05/02.
//
#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TASK *task = task_now();
    int fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;

    fifo32_init(&task->fifo, 128, fifobuf, task);

    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // プロンプト表示
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    char s[30], ss[30], cmdline[30], *p;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i, x, y;
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
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
                    } else if (mystrcmp(cmdline, "ls") == 0) {
                        // ls コマンド
                        for (x = 0; x < 224; ++x) {
                            if (finfo[x].name[0] == 0x00) {
                                // ファイル名の1文字目が0の場合、それ以上ファイルはない、という意味
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                // ファイル名の1文字目が0xe5の場合、そのファイルは削除済み、という意味
                                if ((finfo[x].type & 0x18) == 0) {
                                    mysprintf(s, "filename.ext %d %d", finfo[x].size, finfo[x].clustno);
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
                    } else if (myindexof(cmdline, "cat ") == 0) {
                        // cat コマンド

                        // ファイル名の準備
                        for (y = 0; y < 11; ++y) {
                            // ファイル名は、名前8文字 + 拡張子3文字。あまった文字はスペース。ドットはなし。
                            s[y] = ' ';
                        }
                        s[11] = 0;
                        y = 0;
                        for (x = 4; y < 11 && cmdline[x] != 0; ++x) {
                            if (cmdline[x] == '.' && y <= 8) {
                                y = 8;
                            } else {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    // 小文字は大文字に
                                    s[y] -= 0x20;
                                }
                                y++;
                            }
                        }
                        // ファイルを探す
                        for (x = 0; x < 224; ++x) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                if (myhasprefix(finfo[x].name, s)) {
                                    break;
                                }
                            }
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00) {
                            // ファイルが見つかった場合
                            p = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;
                            for (y = 0; y < finfo[x].size; ++y) {
                                // 1文字ずつ出力
                                s[0] = p[y];
                                s[1] = 0;
                                switch (s[0]) {
                                    case 0x09: // tab
                                        while (1) {
                                            putfonts8_asc_sht(
                                                    sheet,
                                                    cursor_x,
                                                    cursor_y,
                                                    COL8_FFFFFF,
                                                    COL8_000000,
                                                    " ",
                                                    1
                                            );
                                            cursor_x += 8;
                                            if (cursor_x == 8 + 240) {
                                                // 右端まで来たので改行
                                                cursor_x = 8;
                                                cursor_y = cons_newline(cursor_y, sheet);
                                            }
                                            if (((cursor_x - 8) & 0x1f) == 0) {
                                                break;
                                            }
                                        }
                                        break;
                                    case 0x0a: // 改行
                                        cursor_x = 8;
                                        cursor_y = cons_newline(cursor_y, sheet);
                                        break;
                                    case 0x0d: // 復帰
                                        // なにもしない
                                        break;
                                    default: // 普通の文字
                                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                        cursor_x += 8;
                                        if (cursor_x == 8 + 240) {
                                            // 右端まで来たので改行
                                            cursor_x = 8;
                                            cursor_y = cons_newline(cursor_y, sheet);
                                        }
                                }
                            }
                            memman_free_4k(memman, (int) p, finfo[x].size);
                        } else {
                            // ファイルがみつからなかった場合
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
                            cursor_y = cons_newline(cursor_y, sheet);
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
