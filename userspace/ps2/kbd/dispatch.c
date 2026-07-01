#include <uapi/kbd.h>
#include <uapi/uapi.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/ps2/kbd/dispatch.h>
#include <userspace/ps2/kbd/driver.h>

KBD_SERVER_OPS(SERVER_OP_DECL)

const server_handler_f kbd_handlers[SERVER_OP_MAX] = {
    KBD_SERVER_OPS(SERVER_OP_ENTRY)
};

static i32 handle_read(ipc_packet_s* packet) {
    kbd_event_s event;
    ps2_kbd_read(&event);
    
    kbd_server_reply_s* rep = (kbd_server_reply_s*)packet->payload;
    *rep = (kbd_server_reply_s) {
        .ret   = E_OK,
        .event = event
    };

    return (i32)sizeof(kbd_server_reply_s);
}

i32 init(void) {
    ps2_kbd_init();
    printf("[PS/2 Keyboard] Initialized userspace PS/2 keyboard server\n");
    return E_OK;
}

void fini(void) {
    ;
}
