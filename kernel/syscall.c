#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"

#define syscalls(X)          \
    X(0, ipc_send,        2) \
    X(1, ipc_recv,        1) \
    X(2, ipc_call,        2) \
    X(3, ipc_reply,       2) \
    X(4, thread_create,   2) \
    X(5, thread_yield,    0) \
    X(6, thread_exit,     1) \
    X(7, thread_get_self, 0) \
    X(8, task_get_self,   0) \
    X(9, max,             0)

#define syscall_no_enum(id, name, argc) SYS_##name = (id),
typedef enum syscall_no_t {
    syscalls(syscall_no_enum)
} syscall_no_t;
#undef syscall_no_enum

#define syscall_packed(id, name, argc) _Static_assert((SYS_##name) == (id), "Error: '" #name "' is not at position " #id);
syscalls(syscall_packed)
#undef syscall_packed

_Static_assert(SYS_max < 32, "Error: Too many syscalls defined");

#define syscall_decl_0(name) static i32 sys_##name(void)
#define syscall_decl_1(name) static i32 sys_##name(u32)
#define syscall_decl_2(name) static i32 sys_##name(u32, u32)
#define syscall_decl_3(name) static i32 sys_##name(u32, u32, u32)
#define syscall_decl_4(name) static i32 sys_##name(u32, u32, u32, u32)
#define syscall_fwd_decl(id, name, argc) syscall_decl_##argc(name);
syscalls(syscall_fwd_decl)
#undef syscall_fwd_decl

#define syscall_args_0(frm)
#define syscall_args_1(frm) (frm)->ebx
#define syscall_args_2(frm) (frm)->ebx, (frm)->ecx
#define syscall_args_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define syscall_args_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi

static i32 sys_ipc_send(u32 dst, u32 msg) {
    return ipc_send(dst, (const ipc_msg_t*)msg);
}

static i32 sys_ipc_recv(u32 msg) {
    return ipc_recv((ipc_msg_t*)msg);
}

static i32 sys_ipc_call(u32 dst, u32 msg) {
    return ipc_call(dst, (ipc_msg_t*)msg);
}

static i32 sys_ipc_reply(u32 client, u32 msg) {
    return ipc_reply(client, (const ipc_msg_t*)msg);
}

static i32 sys_thread_create(u32 entry, u32 priority) {
    if (priority >= SCHED_PRIORITY_CNT)
        return -(i32)E_INVAL;

    thread_ctrl_blk_t* thread = thread_create(
        thread_get_self()->task, (thread_entry_func_t)entry, (u8)priority
    );

    if (!thread)
        return -(i32)E_NOMEM;
    return (i32)thread->tid;
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

static i32 sys_thread_get_self(void) {
    return (i32)thread_get_self()->tid;
}

static i32 sys_task_get_self(void) {
    return (i32)thread_get_self()->task->id;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

void syscall_handler(syscall_frm_t* frm) {
    static const void* const dispatch[SYS_max + 1] __aligned(64) = {
        #define syscall_lbl(id, name, argc) [id] = &&l_##name,
        syscalls(syscall_lbl)
        #undef syscall_lbl
    };

    thread_get_self()->frm = frm;

    u32 syscall_no = frm->eax;
    syscall_no = likely(syscall_no < SYS_max) ? syscall_no : SYS_max;
    goto *dispatch[syscall_no];

    #define syscall(id, name, argc)                               \
        l_##name:                                                 \
            frm->eax = (u32)sys_##name(syscall_args_##argc(frm)); \
            return;
    syscalls(syscall)
    #undef syscall
}
