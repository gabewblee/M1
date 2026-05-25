#include "servers/vga.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

#define VGA_BUF_SZ (sizeof(((vga_server_req_t*)0)->buf))

static i32 vga_task_id = -(i32)E_INVAL;

static i32 call_vga_server(vga_server_op_t op, const vga_server_req_t* req) {
    if (vga_task_id < 0)
        vga_task_id = sys_server_lookup(VGA_SERVER_NAME);

    ipc_msg_t msg = (ipc_msg_t) {
        .id = (u32)op,
        .sz = (u32)sizeof(vga_server_req_t)
    };
    memcpy(msg.data, req, sizeof(vga_server_req_t));

    i32 ret = sys_ipc_call((u32)vga_task_id, &msg);
    if (ret != E_OK)
        return ret;

    if (msg.sz >= sizeof(i32))
        memcpy(&ret, msg.data, sizeof(i32));

    return ret;
}

static i32 print_unsigned_int(u32 val, u32 base, i32 uppercase) {
    char buf[32]; size_t n = 0; i32 cnt = 0;
    if (val == 0)
        return putc('0');

    while (val > 0) {
        u32 digit = val % base;
        val /= base;
        buf[n++] = (char)(digit < 10 ? '0' + digit : (uppercase ? 'A' : 'a') + digit - 10);
    }

    while (n > 0) {
        i32 ret = putc(buf[--n]);
        if (ret != E_OK)
            return ret;

        cnt++;
    }

    return cnt;
}

static i32 print_signed_int(i32 val) {
    if (val < 0) {
        i32 ret = putc('-');
        if (ret != E_OK)
            return ret;

        ret = print_unsigned_int((u32)(-(val + 1)) + 1u, 10, 0);
        return (ret < 0) ? ret : ret + 1;
    }

    return print_unsigned_int((u32)val, 10, 0);
}

static i32 vprintf(const char* fmt, va_list ap) {
    i32 cnt = 0;
    while (*fmt != '\0') {
        if (*fmt != '%') {
            i32 ret = putc(*fmt++);
            if (ret != E_OK)
                return ret;

            cnt++;
            continue;
        }

        fmt++;
        switch (*fmt++) {
            case '%':
                if (putc('%') != E_OK)
                    return -(i32)E_FAULT;

                cnt++;
                break;
            case 's': {
                const char* str = va_arg(ap, const char*);
                if (!str)
                    str = "(null)";
                
                i32 ret = write(str, strlen(str));
                if (ret < 0)
                    return ret;

                cnt += ret;
                break;
            }
            case 'c': {
                int c = va_arg(ap, int);
                if (putc(c) != E_OK)
                    return -(i32)E_FAULT;

                cnt++;
                break;
            }
            case 'd': {
                i32 ret = print_signed_int(va_arg(ap, i32));
                if (ret < 0)
                    return ret;

                cnt += ret;
                break;
            }
            case 'u': {
                i32 ret = print_unsigned_int(va_arg(ap, u32), 10, 0);
                if (ret < 0)
                    return ret;

                cnt += ret;
                break;
            }
            case 'x': {
                i32 ret = print_unsigned_int(va_arg(ap, u32), 16, 0);
                if (ret < 0)
                    return ret;
                
                cnt += ret;
                break;
            }
            default:
                return -(i32)E_INVAL;
        }
    }

    return cnt;
}

i32 putc(int c) {
    vga_server_req_t req = {
        .len = 1,
        .buf = {(char)c},
    };

    return call_vga_server(VGA_SERVER_OP_putc, &req);
}

i32 puts(const char* str) {
    i32 ret = write(str, strlen(str));
    if (ret < 0)
        return ret;

    if (putc('\n') != E_OK)
        return -(i32)E_FAULT;

    return ret + 1;
}

i32 write(const char* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        size_t chunk = len - total;
        if (chunk > VGA_BUF_SZ)
            chunk = VGA_BUF_SZ;

        vga_server_req_t req = {0};
        req.len = (u8)chunk;
        memcpy(req.buf, buf + total, chunk);

        i32 ret = call_vga_server(VGA_SERVER_OP_write, &req);
        if (ret != E_OK)
            return ret;

        total += chunk;
    }

    return (i32)total;
}

i32 printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    i32 ret = vprintf(fmt, ap);
    va_end(ap);
    return ret;
}
