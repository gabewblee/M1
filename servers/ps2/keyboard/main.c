#include "dispatch.h"

#include "../../libc/server.h"

static const server_t server = {
    .name     = "PS/2 keyboard",
    .init     = init,
    .dispatch = dispatch,
    .fini     = fini,
};

int main(void) {
    run(&server);
}