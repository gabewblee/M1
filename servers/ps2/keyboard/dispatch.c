#include "dispatch.h"
#include "driver.h"

#include "../../libc/stdio.h"

i32 init(void) {
    ps2_keyboard_init();
    printf("[PS/2 keyboard] Initialized userspace PS/2 keyboard server\n");
    return E_OK;
}

i32 dispatch(ipc_msg_t* msg) {
    (void)msg;
    return -(i32)E_NOSYS;
}

void fini(void) {
    ;
}