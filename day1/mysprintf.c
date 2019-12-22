#include <stdarg.h>

int dec2asc(char *str, int dec) {
    int len = 0, len_buf;
    int buf[10];
    while (1) {
        buf[len++] = dec % 10;
        if (dec < 10) {
            break;
        }
        dec /= 10;
    }
    len_buf = len;
    while (len) {
        *(str++) = buf[--len] + 0x30;
    }
    return len_buf;
}

int hex2asc(char *str, int hex) {
    int len = 0, len_buf;
    int buf[10];
    *(str++) = '0';
    *(str++) = 'x';
    while (1) {
        buf[len++] = hex % 0x10;
        if (hex < 0x10) {
            break;
        }
        hex /= 0x10;
    }
    len_buf = len;
    while (len) {
        len--;
        if (buf[len] < 10) {
            *(str++) = buf[len] + 0x30;
        } else {
            *(str++) = buf[len] + 0x57;
        }
    }
    return len_buf + 2;
}

void mysprintf(char *str, char *fmt, ...) {
    va_list args;
    int len = 0;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'x':
                    len = hex2asc(str, va_arg(args,
                    int));
                    break;
                case 'd':
                    len = dec2asc(str, va_arg(args,
                    int));
                    break;
            }
            str += len;
            fmt++;
        } else {
            *(str++) = *(fmt++);
        }
    }
    *str = '\0';
    va_end(args);
}