#include "server.h"
#include "syscall.h"

void run(const server_t* server) {
    i32 ret = server->init();
    if (ret != E_OK)
        sys_thread_exit(ret);

    for (;;) {
        ipc_msg_t msg;
        if (sys_ipc_recv(&msg) != E_OK)
            continue;

        (void)server->dispatch(&msg);
        if (msg.sender != 0)
            (void)sys_ipc_reply(msg.sender, &msg);
    }
    server->fini();
}
