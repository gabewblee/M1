#include <uapi/types.h>
#include <userspace/libc/string.h>

void* memset(void* s, int c, size_t n) {
    u8* ptr = (u8*)s;
    for (size_t i = 0; i < n; i++)
        ptr[i] = (u8)c;

    return s;
}

void* memcpy(void* dst, void* src, size_t n) {
    u8* dest = (u8*)dst;
    u8* from = (u8*)src;
    if ((((uintptr_t)dest | (uintptr_t)from) & 3u) == 0) {
        size_t words = n >> 2;

        for (size_t i = 0; i < words; i++)
            ((u32*)dest)[i] = ((u32*)from)[i];

        dest += words << 2;
        from += words << 2;
        n -= words << 2;
    }

    for (size_t i = 0; i < n; i++)
        dest[i] = from[i];

    return dst;
}

int strcmp(char* a, char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

size_t strlen(char* s) {
    size_t n = 0;
    while (s[n] != '\0')
        n++;

    return n;
}