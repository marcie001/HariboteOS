//
// Created by marcie on 2020/04/25.
//

int mystrcmp(unsigned char *s1, unsigned char *s2) {
    if (s1 == s2) {
        return 0;
    }
    char c;
    while (1) {
        c = *s1 - *s2;
        if (c != 0 || *s1 == 0 || *s2 == 0) {
            return c;
        }
        s1++;
        s2++;
    }
}