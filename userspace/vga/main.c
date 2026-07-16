#include <uapi/bootinfo.h>
#include <userspace/server/server.h>
#include <userspace/vga/dispatch.h>

SERVER_DEF(
    SERVER_ID_vga,
    1,
    3,
    RES_DEV_FRM(SERVICE_CPTR_VGA_FB, BOOT_SLOT_VGA_UNTYPED)
);

static server_s server = {
    .name     = "vga",
    .init     = init,
    .fini     = fini,
    .handlers = vga_handlers,
};

int main(void) {
    run(&server);
}