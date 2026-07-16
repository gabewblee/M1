#include <userspace/ps2/kbd/dispatch.h>
#include <userspace/server/server.h>

SERVER_DEF(
    SERVER_ID_keyboard,
    1, 
    3,
    RES_NTFN(SERVICE_CPTR_NTFN),
    RES_IRQ(SERVICE_CPTR_IRQ, 1),
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga)
);

static server_s server = {
    .name     = "PS/2 keyboard",
    .init     = init,
    .fini     = fini,
    .handlers = kbd_handlers,
};

int main(void) {
    run(&server);
}