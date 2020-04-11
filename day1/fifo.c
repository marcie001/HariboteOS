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
 * @param task データが入ったときに起こすタスク
 */
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    fifo->task = task;
    return;
}

/**
 * FIFO に data を追加する。
 * @param fifo FIFO
 * @param data 追加する data
 * @return 追加したdata. ただし、空きがなく追加できなかったときは -1
 */
int fifo32_put(struct FIFO32 *fifo, int data) {
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
    if (fifo->task != 0) {
        if (fifo->task->flags != 2) {
            task_run(fifo->task);
        }
    }
    return data;
}

/**
 * FIFO からデータを取り出す。
 * @param fifo FIFO
 * @return 取り出したデータ。ただし、 FIFO が空のときは -1
 */
int fifo32_get(struct FIFO32 *fifo) {
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
int fifo32_status(struct FIFO32 *fifo) {
    return fifo->size - fifo->free;
}