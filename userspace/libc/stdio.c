#include <uapi/errno.h>
#include <uapi/servers.h>
#include <uapi/vga.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

#define PRINTF_MAX 256u
#define VGA_BUF_SZ (sizeof(((vga_server_req_s*)0)->buf))

typedef struct fmt_sink_s {
    char*  buf;  /* Destination buffer            */
    size_t size; /* Destination capacity in bytes */
    size_t used; /* Characters emitted so far     */
} fmt_sink_s;

static void sink_putc(fmt_sink_s* sink, char c) {
    if (sink->used + 1u < sink->size)
        sink->buf[sink->used] = c;

    sink->used++;
}

static void sink_pad(fmt_sink_s* sink, char pad, u32 width, u32 len) {
    for (u32 i = len; i < width; i++)
        sink_putc(sink, pad);
}

static void sink_unsigned(fmt_sink_s* sink, u32 value, u32 base, char pad, u32 width) {
    char digits[16];
    u32 n = 0;
    do {
        u32 digit = value % base;
        digits[n++] = (char)(digit < 10u ? '0' + digit : 'a' + digit - 10u);
        value /= base;
    } while (value);

    sink_pad(sink, pad, width, n);
    while (n)
        sink_putc(sink, digits[--n]);
}

static void sink_signed(fmt_sink_s* sink, i32 value, char pad, u32 width) {
    u32 magnitude = (u32)value;
    u32 len = 1;
    if (value < 0) {
        magnitude = ~(u32)value + 1u;
        len++;
    }

    for (u32 rest = magnitude; rest >= 10u; rest /= 10u)
        len++;

    if (value < 0 && pad == '0') {
        sink_putc(sink, '-');
        sink_pad(sink, pad, width, len);
        sink_unsigned(sink, magnitude, 10, ' ', 0);
        return;
    }

    sink_pad(sink, pad, width, len);
    if (value < 0)
        sink_putc(sink, '-');

    sink_unsigned(sink, magnitude, 10, ' ', 0);
}

i32 vsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
    fmt_sink_s sink = { buf, size, 0 };
    while (*fmt) {
        if (*fmt != '%') {
            sink_putc(&sink, *fmt++);
            continue;
        }

        fmt++;
        char pad = ' ';
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }

        u32 width = 0;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10u + (u32)(*fmt++ - '0');

        switch (*fmt++) {
            case '%':
                sink_putc(&sink, '%');
                break;

            case 'c':
                sink_pad(&sink, ' ', width, 1);
                sink_putc(&sink, (char)va_arg(args, int));
                break;

            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s)
                    s = "(null)";

                sink_pad(&sink, ' ', width, (u32)strlen(s));
                while (*s)
                    sink_putc(&sink, *s++);

                break;
            }

            case 'd':
                sink_signed(&sink, va_arg(args, i32), pad, width);
                break;

            case 'u':
                sink_unsigned(&sink, va_arg(args, u32), 10, pad, width);
                break;

            case 'x':
                sink_unsigned(&sink, va_arg(args, u32), 16, pad, width);
                break;

            default:
                sink_putc(&sink, '?');
                break;
        }
    }

    if (size)
        buf[min(sink.used, size - 1u)] = '\0';

    return (i32)sink.used;
}

i32 snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    i32 n = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

static i32 call_vga_server(vga_server_op_e op, vga_server_req_s* req) {
    ipc_msg_s msg;
    memcpy(msg.payload, req, sizeof(vga_server_req_s));

    msg_info_t mi = mk_msg_info((u32)op, 0, (u32)sizeof(vga_server_req_s));
    i32 rep = sys_call(SERVICE_CPTR_VGA, mi, &msg, 0);
    if (rep < 0)
        return rep;

    i32 status = E_OK;
    if (get_msg_len((msg_info_t)rep) >= sizeof(i32))
        memcpy(&status, msg.payload, sizeof(i32));

    return status;
}

i32 putc(int c) {
    vga_server_req_s req = {
        .len = 1,
        .buf = {(char)c},
    };

    return call_vga_server(VGA_SERVER_OP_putc, &req);
}

i32 puts(const char* str) {
    i32 ret = write(str, strlen(str));
    if (ret < 0)
        return ret;

    i32 nl = putc('\n');
    if (nl != E_OK)
        return nl;

    return ret + 1;
}

i32 write(const char* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        size_t chunk = len - total;
        if (chunk > VGA_BUF_SZ)
            chunk = VGA_BUF_SZ;

        vga_server_req_s req = {0};
        req.len = (u8)chunk;
        memcpy(req.buf, buf + total, chunk);

        i32 ret = call_vga_server(VGA_SERVER_OP_write, &req);
        if (ret != E_OK)
            return ret;

        total += chunk;
    }

    return (i32)total;
}

i32 clear(void) {
    vga_server_req_s req = {0};
    return call_vga_server(VGA_SERVER_OP_clear, &req);
}

i32 printf(const char* fmt, ...) {
    char buf[PRINTF_MAX];
    va_list args;
    va_start(args, fmt);
    i32 n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n < 0)
        return n;

    return write(buf, min((size_t)n, sizeof(buf) - 1u));
}

i32 dprintf(const char* fmt, ...) {
    char buf[PRINTF_MAX];
    va_list args;
    va_start(args, fmt);
    i32 n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n < 0)
        return n;

    return sys_dbg_puts(buf, min((u32)n, (u32)sizeof(buf) - 1u));
}
