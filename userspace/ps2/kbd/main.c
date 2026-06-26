#include <userspace/ps2/kbd/dispatch.h>
#include <userspace/server/server.h>

SERVER_DEF(SERVER_ID_keyboard, 1, 3);

static server_s server = {
    .name     = "PS/2 keyboard",
    .init     = init,
    .fini     = fini,
    .handlers = kbd_handlers,
};

int main(void) {
    run(&server);
}