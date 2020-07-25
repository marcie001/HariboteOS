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

int myindexof(unsigned char *str, unsigned char *substr) {
    unsigned char *sub = substr;
    int pos = 0;
    for (int i = 0;; ++i) {
        if (*sub == 0) {
            return pos;
        }
        if (*str == 0) {
            return -1;
        }
        if (*str != *sub) {
            str++;
            sub = substr;
            pos = -1;
            continue;
        }
        if (pos == -1) {
            pos = i;
        }
        str++;
        sub++;
    }
}

int myhasprefix(unsigned char *str, unsigned char *prefix) {
    while (1) {
        if (*str == 0 && *prefix == 0) {
            return 1;
        }
        if (*prefix == 0) {
            return 1;
        }
        if (*str == 0) {
            return 0;
        }
        if (*str != *prefix) {
            return 0;
        }
        str++;
        prefix++;
    }
}

int mymemcmp(unsigned char *s1, unsigned char *s2, int n) {
    for (int i = 0; i < n; ++i) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

int mystrlen(unsigned char *s) {
    int i;
    for (i = 0;; i++) {
        if (*s == 0) {
            return i;
        }
        s++;
    }
}
