#include "libapi.h"

void HariMain(void) {
    static unsigned char s[34] = {
            0xb2, 0xdb, 0xca, 0xc6, 0xce, 0xcd, 0xc4, 0x0a,
            0x82, 0xa2, 0x82, 0xeb, 0x82, 0xcd, 0x82, 0xc9,
            0x82, 0xd9, 0x82, 0xd6, 0x82, 0xc6, 0x82, 0xbf,
            0x82, 0xe8, 0x82, 0xca, 0x82, 0xe9, 0x82, 0xf0,
            0x0a, 0x00,
    };
    api_putstr0(s);
    api_end();
}