#include <uapi/errno.h>
#include <uapi/exec.h>
#include <userspace/libc/exec.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

i32 root_spawn(const char* cmdline) {
    root_spawn_req_s req = {0};
    if (strlcpy(req.cmdline, cmdline, sizeof(req.cmdline)) >= sizeof(req.cmdline))
        return -(i32)E_NAMELONG;

    ipc_msg_s msg;
    memcpy(msg.payload, &req, sizeof(req));

    i32 mi = sys_call(SERVICE_CPTR_ROOT, mk_msg_info(ROOT_SERVER_OP_spawn, 0, sizeof(req)), &msg, 0);
    if (mi < 0)
        return mi;

    if (get_msg_len((msg_info_t)mi) < sizeof(i32))
        return -(i32)E_FAULT;

    i32 status;
    memcpy(&status, msg.payload, sizeof(status));
    return status;
}
