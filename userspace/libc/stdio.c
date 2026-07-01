#include <uapi/errno.h>
#include <uapi/servers.h>
#include <uapi/vga.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

#define VGA_BUF_SZ (sizeof(((vga_server_req_s*)0)->buf))

/* Calls the VGA server over the endpoint the kernel installed at SERVICE_CPTR_VGA.
   The operation travels as the message label; the reply's leading word is the
   server's status code. */
static i32 call_vga_server(enum vga_server_op_e op, vga_server_req_s* req) {
    ipc_packet_s packet;
    memcpy(packet.payload, req, sizeof(vga_server_req_s));

    msg_info_t mi  = mk_msg_info((u32)op, 0, (u32)sizeof(vga_server_req_s));
    i32        rep = sys_call(SERVICE_CPTR_VGA, mi, &packet, 0);
    if (rep < 0)
        return rep;

    i32 status = E_OK;
    if (get_msg_len((msg_info_t)rep) >= sizeof(i32))
        memcpy(&status, packet.payload, sizeof(i32));

    return status;
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

static i32 vprintf(char* fmt, va_list ap) {
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
                char* str = va_arg(ap, char*);
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
    vga_server_req_s req = {
        .len = 1,
        .buf = {(char)c},
    };

    return call_vga_server(VGA_SERVER_OP_putc, &req);
}

i32 puts(char* str) {
    i32 ret = write(str, strlen(str));
    if (ret < 0)
        return ret;

    if (putc('\n') != E_OK)
        return -(i32)E_FAULT;

    return ret + 1;
}

i32 write(char* buf, size_t len) {
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

i32 printf(char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    i32 ret = vprintf(fmt, ap);
    va_end(ap);
    return ret;
}
