//
// Created by marcie on 2020/02/24.
//
#include "bootpack.h"

#define SHEET_USE 1

/**
 * シート管理を新しく作成する。
 * @param memman メモリ管理
 * @param vram VRAMのポインタ
 * @param xsize 画面の横サイズ
 * @param ysize 画面の縦サイズ
 * @return 新しく作成したシート管理のポインタ
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // シートは1枚もない
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0; // 未使用マーク
    }
    err:
    return ctl;
}

/**
 * 新しくシートを作成する
 * @param ctl シート管理
 * @return 新しく作成したシートのポインタ
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) {
        if (ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE; // 使用中マーク
            sht->height = -1; // 非表示中
            return sht;
        }
    }
    return 0; // すべてのシートが使用中だった
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
    int h, old = sht->height; // 設定前の高さを記憶

    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht->height = height;

    if (old > height) {
        if (height >= 0) {
            // 間のものを引き上げる
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            // 非表示化
            if (ctl->top > old) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--; // 表示中のシートが1つ減るので、1番上の高さが減る
        }
        sheet_refresh(ctl);
    } else if (old < height) {
        if (old >= 0) {
            // 間のものを押し下げる
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            // 非表示状態から表示へ
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++; // 表示中のシートが1つ増えるので1番上の高さが増える
        }
        sheet_refresh(ctl);
    }
    return;
}

/**
 * 全シートを再描画する
 * @param ctl シート管理
 */
void sheet_refresh(struct SHTCTL *ctl) {
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sht;
    for (h = 0; h <= ctl->top; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        for (by = 0; by < sht->bysize; by++) {
            vy = sht->vy0 + by;
            for (bx = 0; bx < sht->bxsize; bx++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if (c != sht->col_inv) {
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}

/**
 * シートの高さは変えないでシートを移動する
 * @param ctl メモリ管理
 * @param sht 移動するシート
 * @param vx0 移動位置x
 * @param vy0 移動位置y
 */
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        sheet_refresh(ctl);
    }
    return;
}

/**
 * シートを解放する
 * @param ctl シート管理
 * @param sht 解放するシート
 */
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
    if (sht->height >= 0) {
        sheet_updown(ctl, sht, -1);
    }
    sht->flags = 0; // 未使用マーク
    return;
}