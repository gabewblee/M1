#include <uapi/errno.h>
#include <uapi/ipc.h>
#include <uapi/servers.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/server/server.h>

static i32 dispatch(server_s* server, ipc_msg_s* msg, u32 op, u32 badge) {
    if (op < SERVER_OP_MAX && server->handlers[op])
        return server->handlers[op](msg, badge);

    return rep_stat_only(msg, -(i32)E_NOSYS);
}

i32 rep_stat_only(ipc_msg_s* msg, i32 status) {
    memcpy(msg->payload, &status, sizeof(status));
    return (i32)sizeof(status);
}

void run(server_s* server) {
    if (server->init() != E_OK)
        sys_thread_exit(-(i32)E_FAULT);

    ipc_msg_s msg;
    u32 badge = 0;
    i32 mi = sys_recv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, &msg, &badge, 0);
    for (;;) {
        if (mi < 0) {
            mi = sys_recv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, &msg, &badge, 0);
            continue;
        }

        i32 len = dispatch(server, &msg, get_msg_label((msg_info_t)mi), badge);
        if (len < 0)
            len = rep_stat_only(&msg, len);

        msg_info_t reply = mk_msg_info(0, 0, (u32)len);
        mi = sys_replyrecv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, reply, &msg, &badge);
    }

    server->fini();
}
