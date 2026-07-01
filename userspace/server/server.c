#include <uapi/errno.h>
#include <uapi/ipc.h>
#include <uapi/servers.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/server/server.h>

static i32 dispatch(server_s* server, ipc_packet_s* packet, u32 op) {
    if (op < SERVER_OP_MAX && server->handlers[op])
        return server->handlers[op](packet);

    return rep_stat_only(packet, -(i32)E_NOSYS);
}

i32 rep_stat_only(ipc_packet_s* packet, i32 status) {
    memcpy(packet->payload, &status, sizeof(status));
    return (i32)sizeof(status);
}

void run(server_s* server) {
    if (server->init() != E_OK)
        sys_thread_exit(-(i32)E_FAULT);

    /* Block for the first request on the service endpoint, binding the reply
       object so a calling client stays parked until we answer. Thereafter the
       loop replies and waits again in a single ReplyRecv, the hot path seL4
       servers run. */
    ipc_packet_s packet;
    u32          badge = 0;
    i32          mi    = sys_recv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, &packet, &badge, 0);

    for (;;) {
        if (mi < 0) {
            mi = sys_recv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, &packet, &badge, 0);
            continue;
        }

        i32 len = dispatch(server, &packet, get_msg_label((msg_info_t)mi));
        if (len < 0)
            len = rep_stat_only(&packet, len);

        msg_info_t reply = mk_msg_info(0, 0, (u32)len);
        mi = sys_replyrecv(SERVICE_CPTR_EP, SERVICE_CPTR_REPLY, reply, &packet, &badge);
    }

    server->fini();
}
