#include "dispatch.h"
#include "driver.h"
#include "uapi/uapi.h"
#include "uapi/vga.h"

#include "../libc/string.h"
#include "../libc/syscall.h"

static i32 handle_putc(const ipc_msg_s* msg) {
    const vga_server_req_s* req = (const vga_server_req_s*)msg->data;
    vga_putc(req->buf[0]);
    return E_OK;
}

static i32 handle_puts(const ipc_msg_s* msg) {
    const vga_server_req_s* req = (const vga_server_req_s*)msg->data;
    vga_puts(req->buf);
    return E_OK;
}

static i32 handle_write(const ipc_msg_s* msg) {
    const vga_server_req_s* req = (const vga_server_req_s*)msg->data;
    vga_write(req->buf, req->len);
    return E_OK;
}

static i32 handle_clear(const ipc_msg_s* msg) {
    (void)msg;
    vga_clear();
    return E_OK;
}

i32 init(void) {
    if (sys_map_pg(VGA_ADDR, VGA_ADDR, PG_RW_FLAG | PG_USER_FLAG) != E_OK)
        return -(i32)E_FAULT;

    vga_init();
    vga_puts("[VGA] Initialized userspace VGA server\n");
    return E_OK;
}

i32 dispatch(ipc_msg_s* msg) {
    i32 ret;
    switch ((enum vga_server_op_e)msg->id) {
        case VGA_SERVER_OP_putc:
            ret = handle_putc(msg);
            break;
        case VGA_SERVER_OP_puts:
            ret = handle_puts(msg);
            break;
        case VGA_SERVER_OP_write:
            ret = handle_write(msg);
            break;
        case VGA_SERVER_OP_clear:
            ret = handle_clear(msg);
            break;
        default:
            ret = -(i32)E_NOSYS;
            break;
    }

    msg->sz = sizeof(i32);
    memcpy(msg->data, &ret, sizeof(ret));
    return ret;
}

void fini(void) {
    sys_unmap_pg((void*)VGA_ADDR);
}