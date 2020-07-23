//
// Created by marcie on 2020/02/22.
//
#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

/**
 * PS/2 マウスからの割り込み
 * @param esp
 */
void inthandler2c(int *esp) {
    int data;
    io_out8(PIC1_OCW2, 0x64); // IRQ-12受付完了をPIC1に通知
    io_out8(PIC0_OCW2, 0x62); // IRQ-02受付完了をPIC0に通知
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, mousedata0 + data);
    return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

/**
 * mouse有効化
 */
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec) {
    mousefifo = fifo;
    mousedata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // うまくいくと ACK(0xfa) が送信されてくる
    mdec->phase = 0; // マウスの 0xfa を待っている状態
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
    switch (mdec->phase) {
        case 0:
            // マウスの 0xfa を待っている段階
            if (dat == 0xfa) {
                mdec->phase = 1;
            }
            return 0;
        case 1:
            // マウスの1バイト目を待っている状態
            if ((dat & 0xc8) == 0x08) {
                // 正しい1バイト目のときのみ処理を行う
                // こうすることで断線しそうになって取りこぼすようなことがあってもしばらくすれば正常に動くようになる
                mdec->buf[0] = dat;
                mdec->phase = 2;
            }
            return 0;
        case 2:
            mdec->buf[1] = dat;
            mdec->phase = 3;
            return 0;
        case 3:
            mdec->buf[2] = dat;
            mdec->phase = 1;
            mdec->btn = mdec->buf[0] & 0x07;
            mdec->x = mdec->buf[1];
            mdec->y = mdec->buf[2];
            if ((mdec->buf[0] & 0x10) != 0) {
                mdec->x |= 0xffffff00;
            }
            if ((mdec->buf[0] & 0x20) != 0) {
                mdec->y |= 0xffffff00;
            }
            mdec->y = -mdec->y; // マウスではy方向の符号が画面と逆
            return 1;
    }
    return -1; // ここに来ることはないはず
}
