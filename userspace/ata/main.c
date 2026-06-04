#include "dispatch.h"
#include "server.h"

static const server_s server = {
    .name     = "ATA",
    .init     = init,
    .dispatch = dispatch,
    .fini     = fini,
};

int main(void) {
    run(&server);
}