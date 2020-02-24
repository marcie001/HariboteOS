#include "bootpack.h"

#define MEMMAN_ADDR 0x003c0000
#define MEMMAN_FREES 4090 // 4090でメモリ管理領域に約32KB使う

/**
 * メモリ空き情報
 */
struct FREEINFO {
    unsigned int addr, size;
};

/**
 * メモリ管理
 */
struct MEMMAN {
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);

void memman_init(struct MEMMAN *man);

unsigned int memman_total(struct MEMMAN *man);

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    // ハードウェアがいろいろなデータを溜め込んで不具合を起こさないうちに早く割り込みを受け付けられるように
    // まずGDT/IDTを作り直し、PICを初期化してio_stiを呼び出す
    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); // PIC1 とキーボードの接続を許可(11111001)
    io_out8(PIC1_IMR, 0xef); // マウスを許可(11101111)
    init_keyboard();
    enable_mouse(&mdec);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "ABC 123");
    putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "Haribote OS");
    putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "Haribote OS");

    mysprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);

    mysprintf(s, "memory %dMB    free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 80, COL8_FFFFFF, s);

    while (1) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                mysprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 100, 30, 115);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 100, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i) != 0) {
                    mysprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 64, 16, 64 + 8 * 14 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 64, 16, COL8_FFFFFF, s);
                    // マウスカーソルの移動
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); // マウス消す
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    mysprintf(s, "(%d, %d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 座標消す
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 座標書く
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); // マウス書く
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT     0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end) {
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 386 か 486 以降なのか確認
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; // AC-bit = 1
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        // 386 では AC = 1 にしても自動で 0 に戻ってしまう
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT; // AC-bit = 0
    io_store_eflags(eflg);

    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; // キャッシュ禁止
        store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; // キャッシュ許可
        store_cr0(cr0);
    }

    return i;
}

void memman_init(struct MEMMAN *man) {
    man->frees = 0; // 空き情報の個数
    man->maxfrees = 0; // 状況観察用: freesの最大値
    man->lostsize = 0; // 解放に失敗した合計サイズ
    man->losts = 0; // 解放に失敗した回数
    return;
}

/**
 * 空きサイズの合計を報告する
 * @param men メモリ管理
 */
unsigned int memman_total(struct MEMMAN *man) {
    unsigned i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }
    return t;
}

/**
 * メモリを確保する
 * @param man メモリ管理
 * @param size 確保するサイズ
 * @return 確保したアドレス
 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
    unsigned int i, a;
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                // free[i]がなくなったので前につめる
                man->frees--;
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1]; // 構造体の代入
                }
            }
            return a;
        }
    }
}

/**
 * メモリを解放する
 * @param man メモリ管理
 * @param addr 解放するアドレス
 * @param size 解放するサイズ
 * @return
 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i, j;
    // まとめやすさを考えると、free[]がaddr順に並んでいる方がいいので、どこに挿入するかをまず決定する
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    if (i > 0) {
        // 前に空き領域がある
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 前の空き領域にまとめられる
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 後ろにも空き領域がある
                if (addr + size == man->free[i].addr) {
                    // 後ろの空き領域にもまとめられる
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0; // 成功終了
        }
    }
    if (i < man->frees) {
        // 後ろに空き領域がある
        if (addr + size == man->free[i].addr) {
            // 後ろの空き領域にまとめられる
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }
    if (man->frees < MEMMAN_FREES) {
        // 後ろにも前にもまとめられる空き領域がない
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; // 最大値を更新
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    // 後ろにずらせなかった
    man->losts++;
    man->lostsize += size;
    return -1; // 失敗終了
}