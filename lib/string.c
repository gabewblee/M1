#include <stdint.h>

#include "string.h"

void* memset(void* str, int c, size_t n) {
    unsigned char* ptr = (unsigned char*)str;
    for (size_t i = 0; i < n; i++)
        ptr[i] = (unsigned char)c;
    
    return str;
}

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* dest = (unsigned char*)dst;
    const unsigned char* source = (const unsigned char*)src;
    if ((((uintptr_t)dest | (uintptr_t)source) & 3u) == 0) {
        size_t words = n >> 2;
        for (size_t i = 0; i < words; i++)
            ((u32*)dest)[i] = ((const u32*)source)[i];
            
        dest += words << 2;
        source += words << 2;
        n -= words << 2;
    }
    for (size_t i = 0; i < n; i++)
        dest[i] = source[i];

    return dst;
}