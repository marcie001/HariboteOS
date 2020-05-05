//
// Created by marcie on 2020/05/05.
//

void api_putchar(int c);

void api_end(void);

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