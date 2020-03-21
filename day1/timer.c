// Created by marcie on 2020/03/15.
//
// タイマ関係
#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_ALLOC 1 // 確保した状態を表す
#define TIMER_FLAGS_USING 2 // タイマ作動中を表す

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.using = 0;
    for (int i = 0; i < MAX_TIMER; ++i) {
        timerctl.timers0[i].flags = 0; // 未使用状態を表す
    }
    return;
}

void inthandler20(int *esp) {
    int i;
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60); // IRQ-00受付完了をPICに通知
    timerctl.count++;
    if (timerctl.next > timerctl.count) {
        return;
    }
    timer = timerctl.t0;
    for (i = 0; i < timerctl.using; ++i) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;
    }
    timerctl.using -= i;
    timerctl.t0 = timer;
    if (timerctl.using > 0) {
        timerctl.next = timerctl.t0->timeout;
    } else {
        timerctl.next = 0xffffffff;
    }
    return;
}

struct TIMER *timer_alloc(void) {
    for (int i = 0; i < MAX_TIMER; ++i) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0; // 空いているタイマがみつからなかった
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timerctl.count + timeout;
    timer->flags = TIMER_FLAGS_USING;
    int e = io_load_eflags();
    io_cli();
    timerctl.using++;
    if (timerctl.using == 1) {
        // 動作中のタイマはこれ1つの場合
        timerctl.t0 = timer;
        timer->next = 0;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    struct TIMER *t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        // 先頭に入れる場合
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    // どこにいれればいいか探す
    struct TIMER *s;
    while (1) {
        s = t;
        t = t->next;
        if (t == 0) {
            break;
        }
        if (timer->timeout <= t->timeout) {
            // sとtの間に入れる場合
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
    // いちばん後ろに入れる場合
    s->next = timer;
    timer->next = 0;
    io_store_eflags(e);
    return;
}
