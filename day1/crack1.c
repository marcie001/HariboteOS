//
// Created by marcie on 2020/05/04.
//
#include "libapi.h"

void HariMain(void) {
    *((char *) 0x00102600) = 0;
    api_end();
}
