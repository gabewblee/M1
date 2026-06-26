#include <uapi/errno.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/server/server.h>

static i32 dispatch(server_s* server, ipc_msg_s* msg) {
    if (msg->id < SERVER_OP_MAX && server->handlers[msg->id])
        return server->handlers[msg->id](msg);

    return rep_stat_only(msg, -(i32)E_NOSYS);
}

i32 rep_stat_only(ipc_msg_s* msg, i32 status) {
    msg->sz = sizeof(status);
    memcpy(msg->data, &status, sizeof(status));
    return status;
}

void run(server_s* server) {
    i32 ret = server->init();
    if (ret != E_OK)
        sys_thread_exit(ret);

    for (;;) {
        ipc_msg_s msg;
        if (sys_ipc_recv(&msg) != E_OK)
            continue;

        dispatch(server, &msg);
        if (msg.sender != 0)
            sys_ipc_reply(msg.sender, &msg);
    }
    
    server->fini();
}
