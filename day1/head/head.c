#include "libapi.h"
#include "libcommon.h"

void HariMain(void) {
    int fh;
    char c, cmdline[30], *p, cnt = 0;
    api_cmdline(cmdline, 30);
    for (p = cmdline; *p > ' '; ++p) {
        // スペースが来るまで読み飛ばす
    }
    for (; *p == ' '; p++) {
        // スペースを読み飛ばす
    }

    fh = api_fopen(p);
    if (fh != 0) {
        while (1) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
            if (c == '\n' && ++cnt >= 3) {
                break;
            }
        }
    } else {
        api_putstr0("File not found.\n");
    }
    api_end();
}