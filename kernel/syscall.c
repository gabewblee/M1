#include "ipc.h"
#include "sched.h"
#include "syscall.h"
#include "task.h"
#include "thread.h"

#define _syscalls(X)        \
    X(0,  ipc_send,      2) \
    X(1,  ipc_recv,      1) \
    X(2,  ipc_call,      2) \
    X(3,  ipc_reply,     2) \
    X(4,  thread_create, 2) \
    X(5,  thread_yield,  0) \
    X(6,  thread_exit,   1) \
    X(7,  thread_self,   0) \
    X(8,  task_self,     0) \
    X(9,  max,           0)

#define _syscall_no_enum(id, name, argc) SYS_##name = (id),
typedef enum syscall_no_t {
    _syscalls(_syscall_no_enum)
} syscall_no_t;
#undef _syscall_no_enum

#define _syscall_packed(id, name, argc) _Static_assert((SYS_##name) == (id), "Error: '" #name "' is not at position " #id);
_syscalls(_syscall_packed)
#undef _syscall_packed

_Static_assert(SYS_max < 32, "Error: Too many syscalls defined");

#define _syscall_decl_0(name) static i32 sys_##name(void)
#define _syscall_decl_1(name) static i32 sys_##name(u32)
#define _syscall_decl_2(name) static i32 sys_##name(u32, u32)
#define _syscall_decl_3(name) static i32 sys_##name(u32, u32, u32)
#define _syscall_decl_4(name) static i32 sys_##name(u32, u32, u32, u32)
#define _syscall_fwd_decl(id, name, argc) _syscall_decl_##argc(name);
_syscalls(_syscall_fwd_decl)
#undef _syscall_fwd_decl

#define _syscall_args_0(frm)
#define _syscall_args_1(frm) (frm)->ebx
#define _syscall_args_2(frm) (frm)->ebx, (frm)->ecx
#define _syscall_args_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define _syscall_args_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi

extern thread_ctrl_blk_t* cur_running_thread;

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
    if (priority >= SCHED_PRIO_CNT)
        return -(i32)E_INVAL;

    thread_ctrl_blk_t* task = thread_create(cur_running_thread->task, (thread_entry_func_t)entry, (u8)priority);
    if (!task)
        return -(i32)E_NOMEM;
    return (i32)task->tid;
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

static i32 sys_thread_self(void) {
    return (i32)cur_running_thread->tid;
}

static i32 sys_task_self(void) {
    return (i32)cur_running_thread->task->id;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

void syscall_handler(syscall_frm_t* frm) {
    static const void* const dispatch[SYS_max + 1] __aligned(64) = {
        #define _syscall_lbl(id, name, argc) [id] = &&l_##name,
        _syscalls(_syscall_lbl)
        #undef _syscall_lbl
    };

    cur_running_thread->frm = frm;

    u32 syscall_no = frm->eax;
    syscall_no = likely(syscall_no < SYS_max) ? syscall_no : SYS_max;
    goto *dispatch[syscall_no];

    #define _syscall(id, name, argc)                               \
        l_##name:                                                  \
            frm->eax = (u32)sys_##name(_syscall_args_##argc(frm)); \
            return;
    _syscalls(_syscall)
    #undef _syscall
}
