//
// Created by marcie on 2020/04/04.
//
#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman) {
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (int i = 0; i < MAX_TASKS; ++i) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }
    for (int i = 0; i < MAX_TASKLEVELS; ++i) {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }
    struct TASK *task = task_alloc();
    task->flags = 2; //動作中マーク
    task->priority = 2;
    task->level = 0;
    task_add(task);
    task_switchsub(); // レベル設定
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);
    return task;
}

struct TASK *task_alloc(void) {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (taskctl->tasks0[i].flags == 0) {
            struct TASK *task = &taskctl->tasks0[i];
            task->flags = 1; // 使用中マーク
            task->tss.eflags = 0x00000202; // IF = 1
            task->tss.eax = 0; // とりあえず 0 にしておく
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return 0;
}

void task_run(struct TASK *task, int level, int priority) {
    // level が0未満の場合はレベルを変更しない
    if (level < 0) {
        level = task->level;
    }
    // priority が 0 以下の場合は優先度を変更しない
    if (priority > 0) {
        task->priority = priority;
    }
    if (task->flags == 2 && task->level != level) {
        // 動作中のレベル変更
        // 一旦removeして次のifを実行させる
        task_remove(task);
    }
    if (task->flags != 2) {
        task->level = level;
        task_add(task);
    }
    taskctl->lv_change = 1; // 次回タスクスイッチのときにレベルを見直す
    return;
}

void task_switch(void) {
    struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_lv];
    struct TASK *now_task = t1->tasks[t1->now];
    t1->now++;
    if (t1->now == t1->running) {
        t1->now = 0;
    }
    if (taskctl->lv_change != 0) {
        task_switchsub();
        t1 = &taskctl->level[taskctl->now_lv];
    }
    struct TASK *new_task = t1->tasks[t1->now];
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task) {
        // タスクが1つしかないときにfarjmpすると、切り替わらないけど、タスクスイッチすることになる。
        // このとき CPU は実行を拒否してひどいことになるので、タスクが2以上あることを確認している。
        farjmp(0, new_task->sel);
    }
    return;
}

void task_sleep(struct TASK *task) {
    if (task->flags == 2) {
        struct TASK *now_task = task_now();
        task_remove(task);
        if (task == now_task) {
            // 自分自身のsleepなのでタスクスウィッチが必要
            task_switchsub();
            now_task = task_now();
            farjmp(0, now_task->sel);
        }
    }
    return;
}

struct TASK *task_now(void) {
    struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_lv];
    return t1->tasks[t1->now];
}

void task_add(struct TASK *task) {
    struct TASKLEVEL *t1 = &taskctl->level[task->level];
    t1->tasks[t1->running] = task;
    t1->running++;
    task->flags = 2; // 動作中
    return;
}

void task_remove(struct TASK *task) {
    int i;
    struct TASKLEVEL *t1 = &taskctl->level[task->level];

    for (i = 0; i < t1->running; ++i) {
        if (t1->tasks[i] == task) {
            break;
        }
    }
    t1->running--;
    if (i < t1->now) {
        t1->now--;
    }
    if (t1->now >= t1->running) {
        // nowがおかしな値になっていたら修正
        t1->now = 0;
    }
    task->flags = 1; // スリープ中
    for (; i < t1->running; i++) {
        t1->tasks[i] = t1->tasks[i + 1];
    }
    return;
}

void task_switchsub(void) {
    int i;
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        if (taskctl->level[i].running > 0) {
            break;
        }
    }
    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}