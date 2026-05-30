#include "dispatch.h"
#include "driver.h"
#include "uapi/kbd.h"
#include "uapi/uapi.h"

#include "../../libc/stdio.h"
#include "../../libc/string.h"

static i32 handle_read(ipc_msg_s* msg) {
    kbd_event_s event;
    ps2_kbd_read(&event);
    
    kbd_server_reply_s* rep = (kbd_server_reply_s*)msg->data;
    *rep = (kbd_server_reply_s) {
        .ret   = E_OK,
        .event = event
    };
    return rep->ret;
}

i32 init(void) {
    ps2_kbd_init();
    printf("[PS/2 Keyboard] Initialized userspace PS/2 keyboard server\n");
    return E_OK;
}

i32 dispatch(ipc_msg_s* msg) {
    i32 ret;
    switch ((enum kbd_server_op_e)msg->id) {
        case KBD_SERVER_OP_read:
            ret = handle_read(msg);
            msg->sz = sizeof(kbd_server_reply_s);
            return ret;
        default:
            ret = -(i32)E_NOSYS;
            msg->sz = sizeof(i32);
            memcpy(msg->data, &ret, sizeof(ret));
            return ret;
    }
}

void fini(void) {
    ;
}