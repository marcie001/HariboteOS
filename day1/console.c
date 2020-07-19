//
// Created by marcie on 2020/05/02.
//
#include "bootpack.h"

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);

void cmd_clear(struct CONSOLE *cons);

void cmd_ls(struct CONSOLE *cons);

void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);

void cmd_exit(struct CONSOLE *cons, int *fat);

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TASK *task = task_now();

    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    task->cons = &cons;

    cons.timer = timer_alloc();
    timer_init(cons.timer, &task->fifo, 1);
    timer_settime(cons.timer, 50);

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
                    timer_init(cons.timer, &task->fifo, 0);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(cons.timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(cons.timer, 50);
            }
            if (i == 2) {
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, 28, cons.cur_x + 7, 43);
                cons.cur_c = -1;
            }
            if (i == 4) {
                cmd_exit(&cons, fat);
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
    } else if (mystrcmp(cmdline, "exit") == 0) {
        cmd_exit(cons, fat);
    } else if (myindexof(cmdline, "start ") == 0) {
        cmd_start(cons, cmdline, memtotal);
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
    char name[18], *p, *q;
    struct TASK *task = task_now();
    int i;
    struct SHTCTL *shtctl;
    struct SHEET *sht;


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
        char s[30];
        int segsiz, datsiz, esp, dathrb;
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        if (finfo->size >= 36 && myhasprefix(p + 4, "Hari") && *p == 0x00) {
            segsiz = *((int *) (p + 0x0000));
            esp = *((int *) (p + 0x000c));
            datsiz = *((int *) (p + 0x0010));
            dathrb = *((int *) (p + 0x0014));
            q = (char *) memman_alloc_4k(memman, segsiz);
            task->ds_base = (int) q;
            // 1 - 2 は dsctbl.c で、 3 - 1002 は mtask.c で使っている
            // task->sel / 8 + 1000 はアプリ用のコードセグメント
            set_segmdesc(gdt + task->sel / 8 + 1000, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
            // task->sel / 8 + 2000 はアプリ用のデータセグメント
            set_segmdesc(gdt + task->sel / 8 + 2000, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
            for (i = 0; i < datsiz; i++) {
                // .hrb ファイル内のデータをデータセグメントにコピー
                q[esp + i] = p[dathrb + i];
            }
            start_app(0x1b, task->sel + 1000 * 8, esp, task->sel + 2000 * 8, &(task->tss.esp0));
            shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
            for (i = 0; i < MAX_SHEETS; ++i) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    // アプリが開きっぱなしにした下敷きを発見
                    sheet_free(sht); // 閉じる
                }
            }
            timer_cancelall(&task->fifo);
            memman_free_4k(memman, (int) q, segsiz);
        } else {
            cons_putstr0(cons, ".hrb file format error.\n");
        }
        memman_free_4k(memman, (int) p, finfo->size);
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

void cmd_exit(struct CONSOLE *cons, int *fat) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now();
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
    timer_cancel(cons->timer);
    memman_free_4k(memman, fat, 4 * 2880);
    io_cli();
    fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768); // 768 - 1023
    io_sti();
    while (1) {
        task_sleep(task);
    }
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal) {
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht = open_console(shtctl, memtotal);
    struct FIFO32 *fifo = &sht->task->fifo;
    sheet_slide(sht, 32, 4);
    sheet_updown(sht, shtctl->top);
    for (int i = 6; cmdline[i] != 0; ++i) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); // Enter 送信
    cons_newline(cons);
    return;
}

/**
 *
 * @param edi
 * @param esi
 * @param ebp
 * @param esp
 * @param ebx
 * @param edx 機能番号
 * @param ecx
 * @param eax
 */
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    struct TASK *task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE *cons = task->cons;
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht;
    int *reg = &eax + 1; // eax の次の番地
    int i;
    /*
     * 保存のための PUSHAD を強引に書き換える。
     * reg[0]: EDI
     * reg[1]: ESI
     * reg[2]: EBP
     * reg[3]: ESP
     * reg[4]: EBX
     * reg[5]: EDX
     * reg[6]: ECX
     * reg[7]: EAX
     */
    switch (edx) {
        case 1:
            // 文字を表示する
            // eax: 表示する文字
            cons_putchar(cons, eax & 0xff, 1);
            break;
        case 2:
            // 文字列を表示する
            // ebx: 表示する文字列の番地
            cons_putstr0(cons, (char *) ebx + ds_base);
            break;
        case 3:
            // ecxに指定した長さだけ文字列を表示する。
            // ebx: 表示する文字列の番地
            // ecx: 表示する文字数
            cons_putstr1(cons, (char *) ebx + ds_base, ecx);
            break;
        case 4:
            // 終了 API
            return &(task->tss.esp0);
        case 5:
            // ウィンドウを表示する
            // ebx: ウィンドウのバッファ
            // ESI: ウィンドウの x 方向の大きさ
            // EDI: ウィンドウの y 方向の大きさ
            // EAX: 透明色
            // ECX: ウィンドウの名前
            sht = sheet_alloc(shtctl);
            sht->task = task;
            sht->flags |= 0x10;
            sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
            make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
            sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
            sheet_updown(sht, shtctl->top);
            reg[7] = (int) sht;
            break;
        case 6:
            // ウィンドウに文字列を表示する
            // EBX: ウィンドウの番号
            // ESI: 表示位置の x 座標
            // EDI: 表示位置の y 座標
            // EAX: 色番号
            // ECX: 表示する文字列の長さ
            // EBP: 表示する文字列の番地
            sht = (struct SHEET *) (ebx & 0xfffffffe);
            putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
            if ((ebx & 1) == 0) {
                sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
            }
            break;
        case 7:
            // ウィンドウに矩形を表示する
            // EBX: ウィンドウの番号
            // EAX: x0
            // ECX: y0
            // ESI: x1
            // EDI: y1
            // EBP: 色番号
            sht = (struct SHEET *) (ebx & 0xfffffffe);
            boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
            if ((ebx & 1) == 0) {
                sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
            }
            break;
        case 8:
            // memman の初期化
            // EBX: memman の番地
            // EAX: memman に管理させる領域の最初の番地
            // ECX: memman に管理させる領域のバイト数
            memman_init((struct MEMMAN *) (ebx + ds_base));
            ecx &= 0xfffffff0; // 16 バイト単位にする
            memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
            break;
        case 9:
            // malloc
            // EBX: memman の番地
            // ECX: 要求のバイト数
            // EAX: 確保した領域の番地
            ecx = (ecx + 0x0f) & 0xfffffff0; // 16 バイト単位に切り上げ
            reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
            break;
        case 10:
            // free
            // EBX: memman の番地
            // EAX: 解放したい領域の番地
            // ECX: 解放したいバイト数
            ecx = (ecx + 0x0f) & 0xfffffff0; // 16 バイト単位に切り上げ
            memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
            break;
        case 11:
            // ウィンドウに点を描画する
            // EBX: ウィンドウの番号
            // ESI: 表示位置の x 座標
            // EDI: 表示位置の y 座標
            // EAX: 色番号
            sht = (struct SHEET *) (ebx & 0xfffffffe);
            sht->buf[sht->bxsize * edi + esi] = eax;
            if ((ebx & 1) == 0) {
                sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
            }
            break;
        case 12:
            // ウィンドウをリフレッシュする
            // EBX: ウィンドウの番号
            // EAX: x0
            // ECX: y0
            // ESI: x1
            // EDI: y1
            sht = (struct SHEET *) ebx;
            sheet_refresh(sht, eax, ecx, esi, edi);
            break;
        case 13:
            // ウィンドウに線を引く
            // EBX: ウィンドウの番号
            // EAX: x0
            // ECX: y0
            // ESI: x1
            // EDI: y1
            // EBP: 色番号
            sht = (struct SHEET *) (ebx & 0xfffffffe);
            hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
            if ((ebx & 1) == 0) {
                sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
            }
            break;
        case 14:
            // ウィンドウを閉じる
            // EBX: ウィンドウの番号
            sheet_free((struct SHEET *) ebx);
            break;
        case 15:
            // キー入力
            // EAX: 0=キー入力がなければ-1を返す。スリープはしない
            //      1=キー入力があるまでスリープする
            // EAX: 入力された文字コード
            while (1) {
                io_cli();
                if (fifo32_status(&task->fifo) == 0) {
                    if (eax != 0) {
                        task_sleep(task);   // fifoが空なのでスリープ
                    } else {
                        io_sti();
                        reg[7] = -1;
                        return 0;
                    }
                }
                i = fifo32_get(&task->fifo);
                io_sti();
                if (i <= 1) {
                    // カーソル用タイマ
                    // アプリ実行中はカーソルが出ないので、いつま次は表示用の1を注文しておく
                    timer_init(cons->timer, &task->fifo, 1); // 次は1
                    timer_settime(cons->timer, 50);
                }
                if (i == 2) {
                    // カーソル ON
                    cons->cur_c = COL8_FFFFFF;
                }
                if (i == 3) {
                    cons->cur_c = -1;
                }
                if (256 <= i) {
                    // キーボードデータ（タスクA経由）など
                    reg[7] = i - 256;
                    return 0;
                }
            }
            break;
        case 16:
            // タイマの取得(alloc)
            // EAX: タイマ番号（OS から返される）
            reg[7] = (int) timer_alloc();
            // 自動キャンセル有効
            ((struct TIMER *) reg[7])->flags2 = 1;
            break;
        case 17:
            // タイマの送信データ設定(init)
            // EBX: タイマ番号
            // EAX: データ
            timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
            break;
        case 18:
            // タイマの時間設定(set)
            // EBX: タイマ番号
            // EAX: 時間
            timer_settime((struct TIMER *) ebx, eax);
            break;
        case 19:
            // タイマの解放(free)
            // EBX: タイマ番号
            timer_free((struct TIMER *) ebx);
            break;
        case 20:
            // BEEP サウンド
            // EAX: 周波数（単位はmHz:ミリヘルツ）
            //      例えば EAX = 440000 にすると 440Hz の音が出る。周波数を 0 にすると消音
            if (eax == 0) {
                i = io_in8(0x61);
                io_out8(0x61, i & 0x0d);
            } else {
                i = 1193180000 / eax;
                io_out8(0x43, 0xb6);
                io_out8(0x42, i & 0xff);
                io_out8(0x42, i >> 8);
                i = io_in8(0x61);
                io_out8(0x61, (i | 0x03) & 0x0f);
            }
            break;
    }
    return 0;
}

/**
 * スタック例外発生時の割り込み処理。
 * @param esp スタック。詳しくは inthandler0d のドキュメント参照。
 * @return 常に1（異常終了）
 */
int *inthandler0c(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
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
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
    mysprintf(s, "EIP = %x\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0);
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col) {
    int i, x, y, len, dx, dy;
    dx = x1 - x0;
    dy = y1 - y0;
    x = x0 << 10;
    y = y0 << 10;
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    if (dx >= dy) {
        len = dx + 1;
        if (x0 > x1) {
            dx = -1024;
        } else {
            dx = 1024;
        }
        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len;
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;
        if (y0 > y1) {
            dy = -1024;
        } else {
            dy = 1024;
        }
        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((y1 - x0 - 1) << 10) / len;
        }
    }
    for (i = 0; i < len; ++i) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }
    return;
}