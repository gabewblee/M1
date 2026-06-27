#include <uapi/errno.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/server/server.h>

static i32 dispatch(server_s* server, ipc_packet_s* packet) {
    if (packet->hdr.op < SERVER_OP_MAX && server->handlers[packet->hdr.op])
        return server->handlers[packet->hdr.op](packet);

    return rep_stat_only(packet, -(i32)E_NOSYS);
}

i32 rep_stat_only(ipc_packet_s* packet, i32 status) {
    packet->hdr.sz = sizeof(status);
    memcpy(packet->payload, &status, sizeof(status));
    return status;
}

void run(server_s* server) {
    i32 ret = server->init();
    if (ret != E_OK)
        sys_thread_exit(ret);

    for (;;) {
        ipc_packet_s packet;
        if (sys_ipc_recv(&packet) != E_OK)
            continue;

        dispatch(server, &packet);
        if (packet.hdr.sender != 0)
            sys_ipc_reply(packet.hdr.sender, &packet);
    }
    
    server->fini();
}
