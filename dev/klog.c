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

void klog_init(void) {
    head = 0;
    sz = 0;
}