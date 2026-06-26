#include <userspace/server/server.h>
#include <userspace/vga/dispatch.h>

SERVER_DEF(SERVER_ID_vga, 1, 3);

static server_s server = {
    .name     = "vga",
    .init     = init,
    .fini     = fini,
    .handlers = vga_handlers,
};

int main(void) {
    run(&server);
}