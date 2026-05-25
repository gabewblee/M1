#include "config.h"
#include "dev/klog.h"
#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "kernel/servers.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "lib/string.h"
#include "mm/page.h"
#include "mm/vmm.h"
#include "uapi/ipc.h"
#include "uapi/sysops.h"

#define SYSCALLS(X)                        \
    X(SYS_ipc_send,      ipc_send,      2) \
    X(SYS_ipc_recv,      ipc_recv,      1) \
    X(SYS_ipc_call,      ipc_call,      2) \
    X(SYS_ipc_reply,     ipc_reply,     2) \
    X(SYS_thread_create, thread_create, 2) \
    X(SYS_thread_yield,  thread_yield,  0) \
    X(SYS_thread_exit,   thread_exit,   1) \
    X(SYS_map_pg,        map_pg,        3) \
    X(SYS_unmap_pg,      unmap_pg,      1) \
    X(SYS_max,           max,           0)

#define SYSCALL_DECL_0(name) static i32 sys_##name(void)
#define SYSCALL_DECL_1(name) static i32 sys_##name(u32)
#define SYSCALL_DECL_2(name) static i32 sys_##name(u32, u32)
#define SYSCALL_DECL_3(name) static i32 sys_##name(u32, u32, u32)
#define SYSCALL_DECL_4(name) static i32 sys_##name(u32, u32, u32, u32)
#define SYSCALL_FWD_DECL(no, name, argc) SYSCALL_DECL_##argc(name);
SYSCALLS(SYSCALL_FWD_DECL)
#undef SYSCALL_FWD_DECL

#define SYSCALL_ARGS_0(frm)
#define SYSCALL_ARGS_1(frm) (frm)->ebx
#define SYSCALL_ARGS_2(frm) (frm)->ebx, (frm)->ecx
#define SYSCALL_ARGS_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define SYSCALL_ARGS_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi

static i32 sysop_exec(ipc_msg_t* msg) {
    const sysop_msg_t* req = (const sysop_msg_t*)msg->data;
    switch (msg->id) {
        case SYSOP_log_read: {
            u32 dst = req->arg0, len = req->arg1, off = req->arg2;
            return (i32)klog_read((char*)dst, (size_t)len, (size_t)off);
        }
        case SYSOP_server_lookup: {
            u32 name = req->arg0, maxlen = req->arg1;

            char buf[SERVER_MAX_NAME_LEN];
            memcpy(buf, (const void*)name, maxlen);
            buf[maxlen] = '\0';

            /* Returns task ID */
            return server_lookup(buf);
        }
        default:
            return -(i32)E_NOSYS;
    }
}

static i32 sys_ipc_send(u32 dst, u32 msg) {
    return (dst == KERNEL_TASK_ID) ? sysop_exec((ipc_msg_t*)msg) : ipc_send(dst, (const ipc_msg_t*)msg);
}

static i32 sys_ipc_recv(u32 msg) {
    return ipc_recv((ipc_msg_t*)msg);
}

static i32 sys_ipc_call(u32 dst, u32 msg) {
    return (dst == KERNEL_TASK_ID) ? sysop_exec((ipc_msg_t*)msg) : ipc_call(dst, (ipc_msg_t*)msg);
}

static i32 sys_ipc_reply(u32 client, u32 msg) {
    return ipc_reply(client, (const ipc_msg_t*)msg);
}

static i32 sys_thread_create(u32 entry, u32 priority) {
    thread_ctrl_blk_t* thread = kernel_thread_create(
        thread_self()->task, (thread_entry_func_t)entry, (u8)priority
    );

    return thread ? (i32)thread->tid : -(i32)E_NOMEM;
}

static i32 sys_thread_yield(void) {
    sched_yield();
    return E_OK;
}

static i32 sys_thread_exit(u32 code) {
    (void)code;
    thread_exit();
    __builtin_unreachable();
}

static i32 sys_map_pg(u32 vaddr, u32 paddr, u32 flags) {
    vmm_map_pg((phys_addr_t)paddr, (virt_addr_t)vaddr, flags);
    return E_OK;
}

static i32 sys_unmap_pg(u32 vaddr) {
    vmm_unmap_pg((virt_addr_t)vaddr);
    return E_OK;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

i32 syscall_handler(syscall_frm_t* frm) {
    static const void* const dispatch[SYS_max + 1] __aligned(64) = {
        #define SYSCALL_LBL(no, name, argc) [no] = &&l_##name,
        SYSCALLS(SYSCALL_LBL)
        #undef SYSCALL_LBL
    };

    /* Currently unused */
    thread_self()->frm = frm;

    u32 syscall_no = frm->eax;
    syscall_no = likely(syscall_no < SYS_max) ? syscall_no : SYS_max;
    goto *dispatch[syscall_no];

    #define SYSCALL(no, name, argc) \
    l_##name:                       \
        return sys_##name(SYSCALL_ARGS_##argc(frm));
    SYSCALLS(SYSCALL)
    #undef SYSCALL
}
