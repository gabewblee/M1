#include "string.h"

void* memset(void* str, int c, size_t n) {
    unsigned char* ptr = (unsigned char*)str;
    for (size_t i = 0; i < n; i++)
        ptr[i] = (unsigned char)c;
    
    return str;
}