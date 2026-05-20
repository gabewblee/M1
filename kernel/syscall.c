#include "dev/klog.h"
#include "mm/page.h"
#include "mm/vmm.h"

#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"

#define SYSCALLS(X)    \
    X(0, ipc_send,  2) \
    X(1, ipc_recv,  1) \
    X(2, ipc_call,  2) \
    X(3, ipc_reply, 2) \
    X(4, max,       0)

_Static_assert(SYS_max < 32, "Error: Too many SYSCALLS defined");

#define SYSCALL_DECL_0(name) static i32 sys_##name(void)              
#define SYSCALL_DECL_1(name) static i32 sys_##name(u32)               
#define SYSCALL_DECL_2(name) static i32 sys_##name(u32, u32)          
#define SYSCALL_DECL_3(name) static i32 sys_##name(u32, u32, u32)     
#define SYSCALL_DECL_4(name) static i32 sys_##name(u32, u32, u32, u32)
#define SYSCALL_FWD_DECL(id, name, argc) SYSCALL_DECL_##argc(name);   
SYSCALLS(SYSCALL_FWD_DECL)
#undef SYSCALL_FWD_DECL

#define SYSCALL_ARGS_0(frm)                             
#define SYSCALL_ARGS_1(frm) (frm)->ebx                 
#define SYSCALL_ARGS_2(frm) (frm)->ebx, (frm)->ecx     
#define SYSCALL_ARGS_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define SYSCALL_ARGS_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi

static i32 sysop_exec(sysop_no_t op, u32 arg0, u32 arg1, u32 arg2, u32 arg3);

static i32 sys_ipc_send(u32 dst, u32 msg) {
    if (dst == KERNEL_TASK_ID) {
        const ipc_msg_t* ipc_msg = (const ipc_msg_t*)msg;
        const sysop_msg_t* req = (const sysop_msg_t*)ipc_msg->data;
        if (ipc_msg->sz < sizeof(sysop_msg_t))
            return -(i32)E_INVAL;

        return sysop_exec((sysop_no_t)ipc_msg->id, req->arg0, req->arg1, req->arg2, req->arg3);
    }

    return ipc_send(dst, (const ipc_msg_t*)msg);
}

static i32 sys_ipc_recv(u32 msg) {
    return ipc_recv((ipc_msg_t*)msg);
}

static i32 sys_ipc_call(u32 dst, u32 msg) {
    if (dst == KERNEL_TASK_ID) {
        ipc_msg_t* ipc_msg = (ipc_msg_t*)msg;
        const sysop_msg_t* req = (const sysop_msg_t*)ipc_msg->data;
        if (ipc_msg->sz < sizeof(sysop_msg_t))
            return -(i32)E_INVAL;

        return sysop_exec((sysop_no_t)ipc_msg->id, req->arg0, req->arg1, req->arg2, req->arg3);
    }

    return ipc_call(dst, (ipc_msg_t*)msg);
}

static i32 sys_ipc_reply(u32 client, u32 msg) {
    return ipc_reply(client, (const ipc_msg_t*)msg);
}

static i32 sysop_exec(sysop_no_t op, u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    (void)arg3;
    switch (op) {
        case SYSOP_thread_create: {
            u32 entry = arg0;
            u32 priority = arg1;
            if (priority >= SCHED_PRIORITY_CNT)
                return -(i32)E_INVAL;

            thread_ctrl_blk_t* thread = kernel_thread_create(
                thread_self()->task, (thread_entry_func_t)entry, (u8)priority
            );

            if (!thread)
                return -(i32)E_NOMEM;

            return (i32)thread->tid;
        }

        case SYSOP_thread_yield:
            sched_yield();
            return E_OK;

        case SYSOP_thread_exit: {
            u32 code = arg0;
            (void)code;
            thread_exit();
            __builtin_unreachable();
        }

        case SYSOP_log_read: {
            u32 dst = arg0;
            u32 len = arg1;
            u32 off = arg2;
            if (len == 0)
                return 0;

            if (dst >= HIGHER_HALF_OFFSET)
                return -(i32)E_FAULT;

            if (len > HIGHER_HALF_OFFSET - dst)
                return -(i32)E_FAULT;

            size_t nbytes = klog_read((char*)dst, (size_t)len, (size_t)off);
            return (i32)nbytes;
        }

        case SYSOP_map_pg: {
            u32 vaddr = arg0;
            u32 paddr = arg1;
            u32 flags = arg2;
            if ((vaddr & (PG_SZ - 1u)) != 0)
                return -(i32)E_INVAL;

            if (vaddr == 0 || vaddr >= HIGHER_HALF_OFFSET)
                return -(i32)E_FAULT;

            if ((paddr & (PG_SZ - 1u)) != 0)
                return -(i32)E_INVAL;

            if (flags & ~(PG_FLAG_RW | PG_FLAG_USER))
                return -(i32)E_INVAL;

            if (!(flags & PG_FLAG_USER))
                return -(i32)E_PERM;

            vmm_map_pg((phys_addr_t)paddr, (virt_addr_t)vaddr, flags);
            return E_OK;
        }

        case SYSOP_unmap_pg: {
            u32 vaddr = arg0;

            if ((vaddr & (PG_SZ - 1u)) != 0)
                return -(i32)E_INVAL;

            if (vaddr == 0 || vaddr >= HIGHER_HALF_OFFSET)
                return -(i32)E_FAULT;

            vmm_unmap_pg((virt_addr_t)vaddr);
            return E_OK;
        }
    }

    return -(i32)E_NOSYS;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

void syscall_handler(syscall_frm_t* frm) {
    static const void* const dispatch[SYS_max + 1] __aligned(64) = {
        #define SYSCALL_LBL(id, name, argc) [id] = &&l_##name,
        SYSCALLS(SYSCALL_LBL)
        #undef SYSCALL_LBL
    };

    thread_self()->frm = frm;

    u32 syscall_no = frm->eax;
    syscall_no = likely(syscall_no < SYS_max) ? syscall_no : SYS_max;
    goto *dispatch[syscall_no];

    #define SYSCALL(id, name, argc)                               \
        l_##name:                                                 \
            frm->eax = (u32)sys_##name(SYSCALL_ARGS_##argc(frm)); \
            return;
    SYSCALLS(SYSCALL)
    #undef SYSCALL
}
