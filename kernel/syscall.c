#include <config.h>
#include <dev/klog.h>
#include <kernel/core/sched.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <kernel/ipc/ipc.h>
#include <kernel/ipc/servers.h>
#include <kernel/irq/irq.h>
#include <kernel/syscall.h>
#include <libk/string.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <kernel/uaccess.h>
#include <uapi/errno.h>
#include <uapi/ipc.h>
#include <uapi/syscall.h>

#define SYSCALLS(X)                                      \
    X(SYS_ipc_send,             ipc_send,             2) \
    X(SYS_ipc_recv,             ipc_recv,             1) \
    X(SYS_ipc_call,             ipc_call,             2) \
    X(SYS_ipc_reply,            ipc_reply,            2) \
    X(SYS_thread_create,        thread_create,        2) \
    X(SYS_thread_yield,         thread_yield,         0) \
    X(SYS_thread_exit,          thread_exit,          1) \
    X(SYS_map_pg,               map_pg,               3) \
    X(SYS_unmap_pg,             unmap_pg,             1) \
    X(SYS_vm_alloc_range,       vm_alloc_range,       2) \
    X(SYS_vm_free_range,        vm_free_range,        2) \
    X(SYS_log_read,             log_read,             3) \
    X(SYS_server_lookup,        server_lookup,        1) \
    X(SYS_irq_register_handler, irq_register_handler, 1) \
    X(SYS_irq_wait_for,         irq_wait_for,         1) \
    X(SYS_vm_copy,              vm_copy,              5) \
    X(SYS_max,                  max,                  0)

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

static i32 sys_ipc_send(u32 dst, u32 packet) {
    return ipc_send(dst, (ipc_packet_s*)packet);
}

static i32 sys_ipc_recv(u32 packet) {
    return ipc_recv((ipc_packet_s*)packet);
}

static i32 sys_ipc_call(u32 dst, u32 packet) {
    return ipc_call(dst, (ipc_packet_s*)packet);
}

static i32 sys_ipc_reply(u32 client, u32 packet) {
    return ipc_reply(client, (ipc_packet_s*)packet);
}

static i32 sys_thread_create(u32 entry, u32 priority) {
    thread_ctrl_blk_s* thread = kernel_thread_create(
        thread_self()->task, (virt_addr_t)entry, (u8)priority
    );

    return thread ? thread->id : -(i32)E_NOMEM;
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

static i32 sys_vm_alloc_range(u32 vaddr, u32 len) {
    (void)vaddr; (void)len;
    return -(i32)E_NOSYS;
}

static i32 sys_vm_free_range(u32 vaddr, u32 len) {
    (void)vaddr; (void)len;
    return -(i32)E_NOSYS;
}

static i32 sys_log_read(u32 dst, u32 len, u32 off) {
    return (i32)klog_read((char*)dst, (size_t)len, (size_t)off);
}

static i32 sys_server_lookup(u32 id) {
    return server_lookup(id);
}

static i32 sys_irq_register_handler(u32 irq) {
    return irq_register_handler((u8)irq);
}

static i32 sys_irq_wait_for(u32 irq) {
    return irq_wait_for((u8)irq);
}

static i32 sys_vm_copy(u32 id, u32 dst, u32 src, u32 len, u32 dir) {
    thread_ctrl_blk_s* peer = thread_lookup(id);
    if (unlikely(!peer))
        return -(i32)E_INVAL;

    phys_addr_t dcr3 = peer->task->cr3, scr3 = thread_self()->task->cr3;
    switch (dir) {
        case VM_COPY_FROM_PEER:
            return copy_between_user(scr3, (void*)src, dcr3, (void*)dst, (size_t)len);
        case VM_COPY_TO_PEER:
            return copy_between_user(dcr3, (void*)dst, scr3, (void*)src, (size_t)len);
        default:
            return -(i32)E_INVAL;
    }
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
