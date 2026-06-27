#include <uapi/uapi.h>
#include <uapi/vga.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/vga/dispatch.h>
#include <userspace/vga/driver.h>

VGA_SERVER_OPS(SERVER_OP_DECL)

const server_handler_f vga_handlers[SERVER_OP_MAX] = {
    VGA_SERVER_OPS(SERVER_OP_ENTRY)
};

static i32 handle_putc(ipc_packet_s* packet) {
    vga_server_req_s* req = (vga_server_req_s*)packet->payload;
    vga_putc(req->buf[0]);
    return rep_stat_only(packet, E_OK);
}

static i32 handle_puts(ipc_packet_s* packet) {
    vga_server_req_s* req = (vga_server_req_s*)packet->payload;
    vga_puts(req->buf);
    return rep_stat_only(packet, E_OK);
}

static i32 handle_write(ipc_packet_s* packet) {
    vga_server_req_s* req = (vga_server_req_s*)packet->payload;
    vga_write(req->buf, req->len);
    return rep_stat_only(packet, E_OK);
}

static i32 handle_clear(ipc_packet_s* packet) {
    vga_clear();
    return rep_stat_only(packet, E_OK);
}

i32 init(void) {
    if (sys_map_pg(VGA_ADDR, VGA_ADDR, PG_RW_FLAG | PG_USER_FLAG) != E_OK)
        return -(i32)E_FAULT;

    vga_init();
    vga_puts("[VGA] Initialized userspace VGA server\n");
    return E_OK;
}

void fini(void) {
    sys_unmap_pg((void*)VGA_ADDR);
}
