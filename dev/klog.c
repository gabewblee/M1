#include "dev/klog.h"

#include "config.h"

static char   buf[KLOG_BUF_SZ];
static u32    head;
static size_t sz;

void klog_putc(const char c) {
    buf[head] = c;
    head = (head + 1) % KLOG_BUF_SZ;
    if (sz < KLOG_BUF_SZ)
        sz++;
}

void klog_puts(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++)
        klog_putc(str[i]);
}

void klog_write(const char* str, const size_t len) {
    for (size_t i = 0; i < len; i++)
        klog_putc(str[i]);
}

void klog_clear(void) {
    head = 0;
    sz = 0;
}

size_t klog_read(char* dst, size_t len, size_t off) {
    if (!dst || off >= sz || len == 0)
        return 0;

    size_t available = sz - off;
    size_t n = (len < available) ? len : available;
    u32 tail = (head + KLOG_BUF_SZ - (u32)sz) % KLOG_BUF_SZ;
    u32 idx = (tail + (u32)off) % KLOG_BUF_SZ;
    for (size_t i = 0; i < n; i++) {
        dst[i] = buf[idx];
        idx = (idx + 1) % KLOG_BUF_SZ;
    }

    return n;
}

size_t klog_size(void) {
    return sz;
}

void klog_init(void) {
    head = 0;
    sz = 0;
}