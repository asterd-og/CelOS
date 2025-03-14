#include <string.h>
#include <stdint.h>
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s2 == *s1)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strlen(const char *s1) {
    int i = 0;
    while (s1[i] != 0) i++;
    return i;
}

char* strtok(char* str, const char* delim) {
    static char* p = NULL;
    if (str != NULL) { p = str; }
    else if (p == NULL) { return NULL; }

    char* start = p;
    while (*p != '\0')  {
        const char* d = delim;
        while (*d != '\0') {
            if (*p == *d) {
                *p = '\0';
                p++;
                if (start == p) {
                    start = p;
                    continue;
                }
                return start;
            }
            d++;
        }
        p++;
    }
    if (start == p) { return NULL; }
    return start;
}
