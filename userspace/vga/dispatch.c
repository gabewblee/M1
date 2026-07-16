#include <uapi/errno.h>
#include <uapi/uapi.h>
#include <uapi/vga.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/vga/dispatch.h>
#include <userspace/vga/driver.h>

VGA_SERVER_OPS(SERVER_OP_DECL)

const server_handler_f vga_handlers[SERVER_OP_MAX] = {
    VGA_SERVER_OPS(SERVER_OP_ENTRY)
};

static i32 handle_putc(ipc_msg_s* msg, u32 badge) {
    vga_server_req_s* req = (vga_server_req_s*)msg->payload;
    vga_putc(req->buf[0]);
    return rep_stat_only(msg, E_OK);
}

static i32 handle_puts(ipc_msg_s* msg, u32 badge) {
    vga_server_req_s* req = (vga_server_req_s*)msg->payload;
    vga_puts(req->buf);
    return rep_stat_only(msg, E_OK);
}

static i32 handle_write(ipc_msg_s* msg, u32 badge) {
    vga_server_req_s* req = (vga_server_req_s*)msg->payload;
    vga_write(req->buf, req->len);
    return rep_stat_only(msg, E_OK);
}

static i32 handle_clear(ipc_msg_s* msg, u32 badge) {
    vga_clear();
    return rep_stat_only(msg, E_OK);
}

i32 init(void) {
    if (map_pg(SERVICE_CPTR_VGA_FB, SERVICE_CPTR_VSPACE, SERVICE_CNODE_DEPTH, VGA_ADDR, PG_RW_FLAG) != E_OK)
        return -(i32)E_FAULT;

    vga_init();
    vga_puts("[VGA] Initialized userspace VGA server\n");
    return E_OK;
}

void fini(void) {
    unmap_pg(SERVICE_CPTR_VGA_FB, SERVICE_CPTR_VSPACE, SERVICE_CNODE_DEPTH, VGA_ADDR);
}
