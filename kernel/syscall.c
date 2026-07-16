#include <config.h>
#include <dev/console.h>
#include <kernel/capability/capability.h>
#include <kernel/capability/endpoint.h>
#include <kernel/capability/invoke.h>
#include <kernel/core/sched.h>
#include <kernel/core/thread.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <uapi/capability.h>
#include <uapi/errno.h>
#include <uapi/ipc.h>
#include <uapi/syscall.h>

#define SYSCALLS(X)                    \
    X(SYS_send,        send,        4) \
    X(SYS_nbsend,      nbsend,      4) \
    X(SYS_call,        call,        4) \
    X(SYS_recv,        recv,        5) \
    X(SYS_nbrecv,      nbrecv,      5) \
    X(SYS_reply,       reply,       3) \
    X(SYS_replyrecv,   replyrecv,   5) \
    X(SYS_signal,      signal,      1) \
    X(SYS_wait,        wait,        2) \
    X(SYS_yield,       yield,       0) \
    X(SYS_thread_exit, thread_exit, 1) \
    X(SYS_dbg_puts,    dbg_puts,    2) \
    X(SYS_max,         max,         0)

#define SYSCALL_DECL_0(name) static i32 sys_##name(void)
#define SYSCALL_DECL_1(name) static i32 sys_##name(u32)
#define SYSCALL_DECL_2(name) static i32 sys_##name(u32, u32)
#define SYSCALL_DECL_3(name) static i32 sys_##name(u32, u32, u32)
#define SYSCALL_DECL_4(name) static i32 sys_##name(u32, u32, u32, u32)
#define SYSCALL_DECL_5(name) static i32 sys_##name(u32, u32, u32, u32, u32)
#define SYSCALL_FWD_DECL(no, name, argc) SYSCALL_DECL_##argc(name);
SYSCALLS(SYSCALL_FWD_DECL)
#undef SYSCALL_FWD_DECL

#define SYSCALL_ARGS_0(frm)
#define SYSCALL_ARGS_1(frm) (frm)->ebx
#define SYSCALL_ARGS_2(frm) (frm)->ebx, (frm)->ecx
#define SYSCALL_ARGS_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define SYSCALL_ARGS_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi
#define SYSCALL_ARGS_5(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi, (frm)->edi

static i32 sys_send(u32 cptr, u32 mi, u32 msg, u32 grant_cptr) {
    return ipc_send(&thread_self()->root, cptr, mi, (ipc_msg_s*)msg, grant_cptr, false);
}

static i32 sys_nbsend(u32 cptr, u32 mi, u32 msg, u32 grant_cptr) {
    return ipc_send(&thread_self()->root, cptr, mi, (ipc_msg_s*)msg, grant_cptr, true);
}

static i32 sys_call(u32 cptr, u32 mi, u32 msg, u32 grant_cptr) {
    thread_ctrl_blk_s* cur = thread_self();
    const capability_s* root = &cur->root;

    cte_s* slot = capability_lookup(root, cptr, capability_depth(root));
    if (unlikely(!slot))
        return -(i32)E_INVAL;

    if (slot->capability.type == CAPABILITY_TYPE_ENDPOINT)
        return ipc_call(root, cptr, mi, (ipc_msg_s*)msg, grant_cptr);

    u32 args[CAPABILITY_INVOKE_ARGC] = {0};
    if (msg) {
        i32 ret = copy_from_user(cur->cr3, args, (void*)msg, sizeof(args));
        if (unlikely(ret != E_OK))
            return ret;
    }

    return capability_invoke(cur, slot, get_msg_label((msg_info_t)mi), args, CAPABILITY_INVOKE_ARGC);
}

static i32 recv_common(u32 cptr, u32 reply_cptr, u32 msg, u32 ubadge, u32 recv_slot, bool nonblock) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 badge = 0;

    i32 ret = ipc_recv(&cur->root, cptr, reply_cptr, (ipc_msg_s*)msg, &badge, recv_slot, nonblock);
    if (ret >= 0 && ubadge)
        copy_to_user(cur->cr3, (void*)ubadge, &badge, sizeof(badge));

    return ret;
}

static i32 sys_recv(u32 cptr, u32 reply_cptr, u32 msg, u32 ubadge, u32 recv_slot) {
    return recv_common(cptr, reply_cptr, msg, ubadge, recv_slot, false);
}

static i32 sys_nbrecv(u32 cptr, u32 reply_cptr, u32 msg, u32 ubadge, u32 recv_slot) {
    return recv_common(cptr, reply_cptr, msg, ubadge, recv_slot, true);
}

static i32 sys_reply(u32 reply_cptr, u32 mi, u32 msg) {
    return ipc_reply(&thread_self()->root, reply_cptr, mi, (ipc_msg_s*)msg);
}

static i32 sys_replyrecv(u32 cptr, u32 reply_cptr, u32 mi, u32 msg, u32 ubadge) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 badge = 0;

    i32 ret = ipc_replyrecv(&cur->root, cptr, reply_cptr, mi, (ipc_msg_s*)msg, &badge);
    if (ret >= 0 && ubadge)
        copy_to_user(cur->cr3, (void*)ubadge, &badge, sizeof(badge));

    return ret;
}

static i32 sys_signal(u32 cptr) {
    return ipc_signal(&thread_self()->root, cptr);
}

static i32 sys_wait(u32 cptr, u32 ubadge) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 badge = 0;

    i32 ret = ipc_wait(&cur->root, cptr, &badge);
    if (ret == E_OK && ubadge)
        copy_to_user(cur->cr3, (void*)ubadge, &badge, sizeof(badge));

    return ret;
}

static i32 sys_yield(void) {
    sched_yield();
    return E_OK;
}

static i32 sys_thread_exit(u32 code) {
    (void)code;
    thread_exit();
    __builtin_unreachable();
}

static i32 sys_dbg_puts(u32 ustr, u32 len) {
    thread_ctrl_blk_s* cur = thread_self();
    char kbuf[129];
    if (len > sizeof(kbuf) - 1)
        len = sizeof(kbuf) - 1;

    if (len && copy_from_user(cur->cr3, kbuf, (void*)ustr, len) != E_OK)
        return -(i32)E_FAULT;

    console_write(ALL_FLAG, kbuf, len);
    return (i32)len;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

i32 syscall_handler(syscall_frm_s* frm) {
    static void* dispatch[SYS_max + 1] __aligned(64) = {
        #define SYSCALL_LBL(no, name, argc) [no] = &&l_##name,
        SYSCALLS(SYSCALL_LBL)
        #undef SYSCALL_LBL
    };

    u32 syscall = frm->eax;
    syscall = likely(syscall < SYS_max) ? syscall : SYS_max;
    goto *dispatch[syscall];

    #define SYSCALL(no, name, argc) \
    l_##name:                       \
        return sys_##name(SYSCALL_ARGS_##argc(frm));
    SYSCALLS(SYSCALL)
    #undef SYSCALL
}
