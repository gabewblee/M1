#include "config.h"
#include "dev/klog.h"

#define KLOG_BUF_MASK (KLOG_BUF_SZ - 1)

_Static_assert((KLOG_BUF_SZ & KLOG_BUF_MASK) == 0, "Error: KLOG_BUF_SZ is not a power of 2");

static char   buf[KLOG_BUF_SZ];
static u32    head;
static size_t sz;

size_t klog_read(char* dst, size_t len, size_t off) {
    size_t available = sz - off;
    size_t nbytes = (len < available) ? len : available;
    u32 tail = (head + KLOG_BUF_SZ - (u32)sz) & KLOG_BUF_MASK;
    u32 idx = (tail + (u32)off) & KLOG_BUF_MASK;
    for (size_t i = 0; i < nbytes; i++) {
        dst[i] = buf[idx];
        idx = (idx + 1) & KLOG_BUF_MASK;
    }

    return nbytes;
}

void klog_putc(const char c) {
    buf[head] = c; head = (head + 1) & KLOG_BUF_MASK;
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
    head = 0; sz = 0;
}

void klog_init(void) {
    klog_clear();
}