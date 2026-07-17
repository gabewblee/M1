#include <userspace/ata/dispatch.h>
#include <userspace/server/server.h>

SERVER_DEF(
    SERVER_ID_ata,
    1,
    3,
    RES_NTFN(SERVICE_CPTR_NTFN),
    RES_IRQ(SERVICE_CPTR_IRQ, 14),
    RES_IRQ(SERVICE_CPTR_IRQ2, 15),
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga),
    RES_SHM_WIN(SHM_WIN_ATA)
);

static server_s server = {
    .name     = "ATA",
    .init     = init,
    .fini     = fini,
    .handlers = ata_handlers,
};

int main(void) {
    run(&server);
}