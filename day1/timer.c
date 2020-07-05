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
    for (int i = 0; i < MAX_TIMER; ++i) {
        timerctl.timers0[i].flags = 0; // 未使用状態を表す
    }
    struct TIMER *t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
    return;
}

void inthandler20(int *esp) {
    char ts = 0;
    io_out8(PIC0_OCW2, 0x60); // IRQ-00受付完了をPICに通知
    timerctl.count++;
    if (timerctl.next > timerctl.count) {
        return;
    }
    struct TIMER *timer = timerctl.t0;
    for (;;) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer) {
            fifo32_put(timer->fifo, timer->data);
        } else {
            ts = 1; // task_timerがタイムアウトした
        }
        timer = timer->next;
    }
    timerctl.t0 = timer;
    timerctl.next = timerctl.t0->timeout;
    if (ts != 0) {
        task_switch();
    }
    return;
}

struct TIMER *timer_alloc(void) {
    for (int i = 0; i < MAX_TIMER; ++i) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            timerctl.timers0[i].flags2 = 0;
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
    struct TIMER *t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        // 先頭に入れる場合
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    while (1) {
        struct TIMER *s = t;
        t = t->next;
        if (timer->timeout <= t->timeout) {
            // sとtの間に入れる場合
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
}

int timer_cancel(struct TIMER *timer) {
    int e;
    struct TIMER *t;
    e = io_load_eflags();
    io_cli(); // 設定中にタイマの状態が変化しないようにするため
    if (timer->flags == TIMER_FLAGS_USING) {
        // 取り消し処理を行う
        if (timer == timerctl.t0) {
            // 先頭だった場合の取り消し処理
            timerctl.t0 = t;
            timerctl.next = t->timeout;
        } else {
            //先頭以外の場合の取り消し処理。timerの1つ前を探す
            t = timerctl.t0;
            while (1) {
                if (t->next == timer) {
                    break;
                }
                t = t->next;
            }
            t->next = timer->next;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(e);
        return 1;
    }
    // 取り消し処理は不要
    io_store_eflags(e);
    return 0;
}

/**
 * タイマをすべてキャンセルする
 * @param fifo FIFO
 */
void timer_cancelall(struct FIFO32 *fifo) {
    int e, i;
    struct TIMER *t;
    e = io_load_eflags();
    io_cli(); // 設定中にタイマの状態が変化しないように
    for (int i = 0; i < MAX_TIMER; ++i) {
        t = &timerctl.timers0[i];
        if (t->flags != 0 && t->flags2 != 0 && t->fifo == fifo) {
            timer_cancel(t);
            timer_free(t);
        }
    }
    io_store_eflags(e);
    return;
}
