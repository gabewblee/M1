#include <userspace/server/server.h>
#include <userspace/vfs/dispatch.h>

SERVER_DEF(
    SERVER_ID_vfs,
    1,
    0,
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga),
    RES_SERV_EP(SERVICE_CPTR_TMPFS, SERVER_ID_tmpfs),
    RES_SERV_EP(SERVICE_CPTR_EXT2, SERVER_ID_ext2)
);

static server_s server = {
    .name     = "VFS",
    .init     = init,
    .fini     = fini,
    .handlers = vfs_handlers,
};

int main(void) {
    run(&server);
}
