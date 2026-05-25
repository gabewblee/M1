#include "dispatch.h"

#include "../lib/server.h"

static const server_t server = {
    .name     = "vga",
    .init     = init,
    .dispatch = dispatch,
    .fini     = fini,
};

int main(void) {
    run(&server);
}
