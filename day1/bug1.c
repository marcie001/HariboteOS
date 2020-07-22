//
// Created by marcie on 2020/05/05.
//
#include "libapi.h"

void HariMain(void) {
    char a[100];
    a[10] = 'A';
    api_putchar(a[10]);
    a[100] = 'B';
    api_putchar(a[100]);
    a[123] = 'C';
    api_putchar(a[123]);
    api_end();
}