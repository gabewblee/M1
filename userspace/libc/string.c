#include <uapi/types.h>
#include <userspace/libc/string.h>

void* memset(void* s, int c, size_t n) {
    u8* ptr = (u8*)s;
    for (size_t i = 0; i < n; i++)
        ptr[i] = (u8)c;

    return s;
}

void* memcpy(void* dst, const void* src, size_t n) {
    u8* dest = (u8*)dst;
    const u8* from = (const u8*)src;
    if ((((uintptr_t)dest | (uintptr_t)from) & 3u) == 0) {
        size_t words = n >> 2;

        for (size_t i = 0; i < words; i++)
            ((u32*)dest)[i] = ((const u32*)from)[i];

        dest += words << 2;
        from += words << 2;
        n -= words << 2;
    }

    for (size_t i = 0; i < n; i++)
        dest[i] = from[i];

    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    u8* dest = (u8*)dst;
    const u8* from = (const u8*)src;
    if (dest <= from || dest >= from + n)
        return memcpy(dst, src, n);

    for (size_t i = n; i > 0; i--)
        dest[i - 1] = from[i - 1];

    return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
    const u8* lhs = (const u8*)a;
    const u8* rhs = (const u8*)b;
    for (size_t i = 0; i < n; i++)
        if (lhs[i] != rhs[i])
            return (int)lhs[i] - (int)rhs[i];

    return 0;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i])
            return (unsigned char)a[i] - (unsigned char)b[i];

        if (a[i] == '\0')
            return 0;
    }

    return 0;
}

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n] != '\0')
        n++;

    return n;
}

char* strchr(const char* s, int c) {
    for (;; s++) {
        if (*s == (char)c)
            return (char*)s;

        if (*s == '\0')
            return NULL;
    }
}

size_t strlcpy(char* dst, const char* src, size_t size) {
    size_t n = strlen(src);
    if (size == 0)
        return n;

    size_t copy = (n < size - 1) ? n : size - 1;
    memcpy(dst, src, copy);
    dst[copy] = '\0';
    return n;
}

size_t strsplit(char* s, char** fields, size_t count) {
    size_t n = 0;
    while (*s && n < count) {
        while (*s == ' ')
            *s++ = '\0';

        if (!*s)
            break;

        fields[n++] = s;
        while (*s && *s != ' ')
            s++;
    }

    if (*s)
        *s = '\0';

    return n;
}
