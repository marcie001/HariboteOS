//
// Created by marcie on 2020/05/02.
//
#include "bootpack.h"

struct CONSOLE {
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
};

void cons_newline(struct CONSOLE *cons);

void cons_putchar(struct CONSOLE *cons, int chr, char move);

void cons_putstr0(struct CONSOLE *cons, char *s);

void cons_putstr1(struct CONSOLE *cons, char *s, int l);

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);

void cmd_clear(struct CONSOLE *cons);

void cmd_ls(struct CONSOLE *cons);

void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TASK *task = task_now();
    int fifobuf[128];

    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    *((int *) 0x0fec) = (int) &cons;

    fifo32_init(&task->fifo, 128, fifobuf, task);

    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // プロンプト表示
    cons_putchar(&cons, '>', 1);

    char cmdline[30];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i;
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
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, 28, cons.cur_x + 7, 43);
                cons.cur_c = -1;
            }
            if (256 <= i && i <= 511) {
                // キーボードデータ
                if (i == 8 + 256) {
                    // バックスペース
                    if (cons.cur_x > 16) {
                        // カーソルをスペースで消してから、カーソルを1つ戻す
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    // Enter
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cur_x / 8 - 2] = 0;
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    // プロンプト表示
                    cons_putchar(&cons, '>', 1);
                } else {
                    // 一般文字
                    if (cons.cur_x < 240) {
                        cmdline[cons.cur_x / 8 - 2] = i - 256;
                        cons_putchar(&cons, i - 256, 1);
                    }
                }
            }
            // カーソル再表示
            if (cons.cur_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7,
                         cons.cur_y + 15);
            }
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
        }
    }
    // return は書かない。 return は呼び出し元への JMP 命令のようなものだから。この関数は呼び出し元がないので。
    // ちなみに、return 先の番地は [ESP] にある
}

void cons_newline(struct CONSOLE *cons) {
    int x, y;
    if (cons->cur_y < 28 + 112) {
        cons->cur_y += 16;
    } else {
        // スクロール
        for (y = 28; y < 28 + 112; ++y) {
            for (x = 8; x < 8 + 240; ++x) {
                cons->sht->buf[x + y * cons->sht->bxsize] = cons->sht->buf[x + (y + 16) * cons->sht->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; ++y) {
            for (x = 8; x < 8 + 240; ++x) {
                cons->sht->buf[x + y * cons->sht->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(cons->sht, 8, 28, 8 + 240, 28 + 128);
    }
    cons->cur_x = 8;
    return;
}

void cons_putchar(struct CONSOLE *cons, int chr, char move) {
    char s[2];
    s[0] = chr;
    s[1] = 0;
    switch (s[0]) {
        case 0x09: // tab
            while (1) {
                putfonts8_asc_sht(
                        cons->sht,
                        cons->cur_x,
                        cons->cur_y,
                        COL8_FFFFFF,
                        COL8_000000,
                        " ",
                        1
                );
                cons->cur_x += 8;
                if (cons->cur_x == 8 + 240) {
                    // 右端まで来たので改行
                    cons_newline(cons);
                }
                if (((cons->cur_x - 8) & 0x1f) == 0) {
                    break;
                }
            }
            break;
        case 0x0a: // 改行
            cons_newline(cons);
            break;
        case 0x0d: // 復帰
            // なにもしない
            break;
        default: // 普通の文字
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
            if (move != 0) {
                // move が 0 以外のときのみカーソルを進める
                cons->cur_x += 8;
                if (cons->cur_x == 8 + 240) {
                    // 右端まで来たので改行
                    cons_newline(cons);
                }
            }
    }
    return;
}

void cons_putstr0(struct CONSOLE *cons, char *s) {
    for (; *s != 0; s++) {
        cons_putchar(cons, *s, 1);
    }
    return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l) {
    for (int i = 0; i < l; i++) {
        cons_putchar(cons, s[i], 1);
    }
    return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
    if (mystrcmp(cmdline, "mem") == 0) {
        cmd_mem(cons, memtotal);
    } else if (mystrcmp(cmdline, "clear") == 0) {
        cmd_clear(cons);
    } else if (mystrcmp(cmdline, "ls") == 0) {
        cmd_ls(cons);
    } else if (myindexof(cmdline, "cat ") == 0) {
        cmd_cat(cons, fat, cmdline);
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) {
            // 存在しないコマンドの実行
            cons_putstr0(cons, "Bad command\n\n");
        }
    }
    return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    char name[13], *p, *q;
    struct TASK *task = task_now();
    int i;

    for (i = 0; i < 13; ++i) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0;
    struct FILEINFO *finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i - 1] != '.') {
        // 後ろに ".HRB" をつけて再検索
        name[i] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }
    if (finfo != 0) {
        p = (char *) memman_alloc_4k(memman, finfo->size);
        q = (char *) memman_alloc_4k(memman, 64 * 1024);
        *((int *) 0xfe8) = (int) p;
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        // 1 - 2 は dsctbl.c で、 3 - 1002 は mtask.c で使っている
        // 1003 はアプリ用のコードセグメント
        set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
        // 1004 はアプリ用のデータセグメント
        set_segmdesc(gdt + 1004, 64 * 1024 - 1, (int) q, AR_DATA32_RW + 0x60);
        if (finfo->size >= 8 && myhasprefix(p + 4, "Hari")) {
            p[0] = 0xe8;
            p[1] = 0x16;
            p[2] = 0x00;
            p[3] = 0x00;
            p[4] = 0x00;
            p[5] = 0xcb;
        }
        start_app(0, 1003 * 8, 64 * 1024, 1004 * 8, &(task->tss.esp0));
        memman_free_4k(memman, (int) p, finfo->size);
        memman_free_4k(memman, (int) q, 64 * 1024);
        cons_newline(cons);
        return 1;
    }
    return 0;
}

/**
 * mem コマンド
 * @param cons
 * @param memtotal
 */
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[60];
    mysprintf(s, "total %dMB\nfree  %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr0(cons, s);
    return;
}

/**
 * clear コマンド
 * @param cons
 */
void cmd_clear(struct CONSOLE *cons) {
    int x, y;
    for (y = 28; y < 28 + 128; ++y) {
        for (x = 8; x < 8 + 240; ++x) {
            cons->sht->buf[x + y * cons->sht->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(cons->sht, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

/**
 * ls コマンド
 * @param cons
 */
void cmd_ls(struct CONSOLE *cons) {
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int x, y;
    char s[30];
    for (x = 0; x < 224; ++x) {
        if (finfo[x].name[0] == 0x00) {
            // ファイル名の1文字目が0の場合、それ以上ファイルはない、という意味
            break;
        }
        if (finfo[x].name[0] != 0xe5) {
            // ファイル名の1文字目が0xe5の場合、そのファイルは削除済み、という意味
            if ((finfo[x].type & 0x18) == 0) {
                mysprintf(s, "filename.ext %d %d\n", finfo[x].size, finfo[x].clustno);
                for (y = 0; y < 8; ++y) {
                    s[y] = finfo[x].name[y];
                }
                s[9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                cons_putstr0(cons, s);
            }
        }
    }
    cons_newline(cons);
    return;
}

void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    char *p;
    if (finfo != 0) {
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, finfo->size);
        memman_free_4k(memman, (int) p, finfo->size);
    } else {
        // ファイルがみつからなかった場合
        cons_putstr0(cons, "File not found.\n");
    }
    cons_newline(cons);
    return;
}

/**
 *
 * @param edi
 * @param esi
 * @param ebp
 * @param esp
 * @param ebx 機能番号1, 2のとき、文字列の番地
 * @param edx 機能番号
 * @param ecx 機能番号2のとき、文字数
 * @param eax 機能番号0のとき、文字
 */
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    int cs_base = *((int *) 0xfe8);
    struct TASK *task = task_now();
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    switch (edx) {
        case 1:
            cons_putchar(cons, eax & 0xff, 1);
            break;
        case 2:
            cons_putstr0(cons, (char *) ebx + cs_base);
            break;
        case 3:
            cons_putstr1(cons, (char *) ebx + cs_base, ecx);
            break;
        case 4:
            return &(task->tss.esp0);
    }
    return 0;
}

/**
 * スタック例外発生時の割り込み処理。
 * @param esp スタック。詳しくは inthandler0d のドキュメント参照。
 * @return 常に1（異常終了）
 */
int *inthandler0c(int *esp) {
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0xfec);
    struct TASK *task = task_now();
    char s[30];
    cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
    mysprintf(s, "EIP = %x\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0);
}

/**
 * 一般保護例外発生時の割り込み処理。
 * @param esp スタック。
 * 0 - 7 は asm_inthandler の PUSHAD の結果
 * 0:   EDI
 * 1:   ESI
 * 2:   EBP
 * 4:   EBX
 * 5    EDX
 * 6:   ECX
 * 7:   EAX
 * 8 - 9 は asm_inthandler の PUSH の結果
 * 8:   DS
 * 9:   ES
 * 10 - 15 は例外発生時に CPU が自動で PUSH した結果
 * 10:  エラーコード
 * 11:  EIP
 * 12:  CS
 * 13:  EFLAGS
 * 14:  ESP（アプリ用）
 * 15:  SS（アプリ用）
 * @return 常に1（異常終了）
 */
int *inthandler0d(int *esp) {
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0xfec);
    struct TASK *task = task_now();
    char s[30];
    cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
    mysprintf(s, "EIP = %x\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0);
}