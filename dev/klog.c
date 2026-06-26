#include <config.h>
#include <dev/klog.h>

#define KLOG_BUF_MASK (KLOG_BUF_SZ - 1)

_Static_assert((KLOG_BUF_SZ & KLOG_BUF_MASK) == 0, "Error: KLOG_BUF_SZ must be a power of two");

static u32    head;
static char   buf[KLOG_BUF_SZ];
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

void klog_putc(char c) {
    buf[head] = c; head = (head + 1) & KLOG_BUF_MASK;
    if (sz < KLOG_BUF_SZ)
        sz++;
}

void klog_puts(char* str) {
    for (size_t i = 0; str[i]; i++)
        klog_putc(str[i]);
}

void klog_write(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        klog_putc(buf[i]);
}

void klog_clear(void) {
    head = sz = 0;
}

void klog_init(void) {
    klog_clear();
}