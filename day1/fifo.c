//
// Created by marcie on 2020/02/09.
//

#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001
/**
 * FIFO の初期化を行う
 * @param fifo 初期化するFIFO
 * @param size バッファのサイズ
 * @param buf バッファのアドレス
 */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

/**
 * FIFO に data を追加する。
 * @param fifo FIFO
 * @param data 追加する data
 * @return 追加したdata. ただし、空きがなく追加できなかったときは -1
 */
int fifo8_put(struct FIFO8 *fifo, unsigned char data) {
    if (fifo->free == 0) {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;
    return data;
}

/**
 * FIFO からデータを取り出す。
 * @param fifo FIFO
 * @return 取り出したデータ。ただし、 FIFO が空のときは -1
 */
int fifo8_get(struct FIFO8 *fifo) {
    int data;
    if (fifo->free == fifo->size) {
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/**
 * FIFO の保存済みデータ量を返す
 * @param fifo FIFO
 * @return データ量
 */
int fifo8_status(struct FIFO8 *fifo) {
    return fifo->size - fifo->free;
}