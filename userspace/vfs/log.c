#include <stdarg.h>
#include <uapi/uapi.h>
#include <userspace/libc/syscall.h>
#include <userspace/vfs/log.h>

#define LOG_MAX 256u

static u32 emit_string(char* buffer, u32 used, const char* s) {
    while (*s && used < LOG_MAX - 1u)
        buffer[used++] = *s++;

    return used;
}

static u32 emit_unsigned(char* buffer, u32 used, u32 value, u32 base) {
    char digits[16];
    u32 n = 0;
    if (value == 0)
        digits[n++] = '0';

    while (value) {
        u32 digit = value % base;
        digits[n++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= base;
    }

    while (n && used < LOG_MAX - 1u)
        buffer[used++] = digits[--n];

    return used;
}

void vfs_log(const char* fmt, ...) {
    char buffer[LOG_MAX];
    u32 used = 0;
    va_list args;
    va_start(args, fmt);
    while (*fmt && used < LOG_MAX - 1u) {
        if (*fmt != '%') {
            buffer[used++] = *fmt++;
            continue;
        }

        fmt++;
        switch (*fmt++) {
            case '%':
                buffer[used++] = '%';
                break;

            case 'c':
                buffer[used++] = (char)va_arg(args, int);
                break;

            case 's':
                used = emit_string(buffer, used, va_arg(args, const char*));
                break;

            case 'u':
                used = emit_unsigned(buffer, used, va_arg(args, u32), 10);
                break;

            case 'x':
                used = emit_unsigned(buffer, used, va_arg(args, u32), 16);
                break;

            case 'd': {
                i32 value = va_arg(args, i32);
                if (value < 0) {
                    buffer[used++] = '-';
                    value = -value;
                }

                used = emit_unsigned(buffer, used, (u32)value, 10);
                break;
            }

            default:
                break;
        }
    }

    va_end(args);
    sys_dbg_puts(buffer, used);
}
