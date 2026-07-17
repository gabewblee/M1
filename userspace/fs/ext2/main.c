#include <userspace/fs/ext2/ext2.h>
#include <userspace/fs/lib/dispatch.h>
#include <userspace/server/server.h>

SERVER_DEF(
    SERVER_ID_ext2,
    1,
    0,
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga),
    RES_SERV_EP(SERVICE_CPTR_ATA, SERVER_ID_ata),
    RES_SHM_WIN(SHM_WIN_ATA)
);

int main(void) {
    fs_server_main(&ext2_driver);
}
