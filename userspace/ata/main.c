#include <userspace/ata/dispatch.h>
#include <userspace/server/server.h>

SERVER_DEF(SERVER_ID_ata, 1, 3);

static server_s server = {
    .name     = "ATA",
    .init     = init,
    .fini     = fini,
    .handlers = ata_handlers,
};

int main(void) {
    run(&server);
}