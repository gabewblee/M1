#include <userspace/fs/lib/dispatch.h>
#include <userspace/fs/tmpfs/tmpfs.h>
#include <userspace/server/server.h>

SERVER_DEF(
    SERVER_ID_tmpfs,
    1,
    0,
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga)
);

int main(void) {
    fs_server_main(&tmpfs_driver);
}
