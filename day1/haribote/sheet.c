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
    ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {
        memman_free_4k(memman, (int) ctl, sizeof(struct SHTCTL));
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // シートは1枚もない
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0; // 未使用マーク
        ctl->sheets0[i].ctl = ctl;
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
            sht->task = 0; // 自動で閉じる機能を使わない
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

void sheet_updown(struct SHEET *sht, int height) {
    struct SHTCTL *ctl = sht->ctl;
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
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        } else {
            // 非表示化
            if (ctl->top > old) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--; // 表示中のシートが1つ減るので、1番上の高さが減る
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        }
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
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }
    return;
}

/**
 * 全シートを再描画する
 * @param ctl シート管理
 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height,
                         sht->height);
    }
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int h, bx, by, bx0, by0, bx1, by1, vx, vy, bx2, sid4, i, i1, *p, *q, *r, j;
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
    struct SHEET *sht;
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 < 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    for (h = h0; h <= h1; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;

        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) {
            bx0 = 0;
        }
        if (by0 < 0) {
            by0 = 0;
        }
        if (bx1 > sht->bxsize) {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
            by1 = sht->bysize;
        }
        if ((sht->vx0 & 3) == 0) {
            // 4バイト型
            i = (bx0 + 3) / 4; // bx0を4で割ったもの（端数切り上げ）
            i1 = bx1 / 4; // bx1を4で割ったもの（端数切り捨て）
            i1 = i1 - i;
            sid4 = sid | sid << 8 | sid << 16 | sid << 24;
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;

                // 前の端数を1バイトずつ処理
                for (bx = bx0; bx < bx1 && (bx & 3) != 0; bx++) {
                    vx = sht->vx0 + bx;
                    if (map[vy * ctl->xsize + vx] == sid) {
                        vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                    }
                }
                vx = sht->vx0 + bx;
                p = (int *) &map[vy * ctl->xsize + vx];
                q = (int *) &vram[vy * ctl->xsize + vx];
                r = (int *) &buf[by * sht->bxsize + bx];

                // 4の倍数部分の処理
                for (i = 0; i < i1; i++) {
                    if (p[i] == sid4) {
                        // おそらく多くの場合はこちらのブロックの処理になるので速い
                        q[i] = r[i];
                    } else {
                        bx2 = bx + i * 4;
                        vx = sht->vx0 + bx2;
                        for (j = 0; j < 4; j++) {
                            if (map[vy * ctl->xsize + vx + j] == sid) {
                                vram[vy * ctl->xsize + vx + j] = buf[by * sht->bxsize + bx2 + j];
                            }
                        }
                    }
                }

                //後ろの端数を1バイトずつ処理
                for (bx += i1 * 4; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    if (map[vy * ctl->xsize + vx] == sid) {
                        vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                    }
                }
            }
            continue;
        }
        // 1バイト型
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                if (map[vy * ctl->xsize + vx] == sid) {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                }
            }
        }
    }
    return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
    unsigned char *buf, sid, *map = ctl->map;
    struct SHEET *sht;
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 < 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    for (h = h0; h <= ctl->top; h++) {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0; // 番地を引き算してそれを下じき番号として利用する
        buf = sht->buf;
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) {
            bx0 = 0;
        }
        if (by0 < 0) {
            by0 = 0;
        }
        if (bx1 > sht->bxsize) {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
            by1 = sht->bysize;
        }
        if (sht->col_inv == -1) {
            // 透明色がない時
            if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) {
                // 4バイトいっぺんに書き込み高速化
                bx1 = (bx1 - bx0) / 4; // MOV命令の回数
                sid4 = sid | sid << 8 | sid << 16 | sid << 24;
                for (by = by0; by < by1; by++) {
                    vy = sht->vy0 + by;
                    vx = sht->vx0 + bx0;
                    p = (int *) &map[vy * ctl->xsize + vx];
                    for (bx = 0; bx < bx1; bx++) {
                        p[bx] = sid4;
                    }
                }
            }
            // 高速化していない
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    map[vy * ctl->xsize + vx] = sid;
                }
            }
            continue;
        }
        // 透明色があるとき
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                if (buf[by * sht->bxsize + bx] != sht->col_inv) {
                    map[vy * ctl->xsize + vx] = sid;
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
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
    struct SHTCTL *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

/**
 * シートを解放する
 * @param ctl シート管理
 * @param sht 解放するシート
 */
void sheet_free(struct SHEET *sht) {
    struct SHTCTL *ctl = sht->ctl;
    if (sht->height >= 0) {
        sheet_updown(sht, -1);
    }
    sht->flags = 0; // 未使用マーク
    return;
}
