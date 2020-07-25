#include "libapi.h"
#include "libcommon.h"

void HariMain(void) {
    int fh;
    unsigned char c;
    char cmdline[30], *p;
    api_cmdline(cmdline, 30);
    for (p = cmdline; *p > ' '; ++p) {
        // スペースが来るまで読み飛ばす
    }
    for (; *p == ' '; p++) {
        // スペースを読み飛ばす
    }

    fh = api_fopen(p);
    int cnt = 0;
    char s[11];
    char *tmpl = "%x\n";
    if (fh != 0) {
        while (1) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            mysprintf(s, tmpl, (int) c);
            api_putstr0(s);
        }
    } else {
        api_putstr0("File not found.\n");
    }
    api_end();
}