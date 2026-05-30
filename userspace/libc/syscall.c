#include "string.h"
#include "syscall.h"

static inline i32 syscall0(u32 no) {
    i32 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(no) : "memory");
    return ret;
}

static inline i32 syscall1(u32 no, u32 a1) {
    i32 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(no), "b"(a1) : "memory");
    return ret;
}

static inline i32 syscall2(u32 no, u32 a1, u32 a2) {
    i32 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(no), "b"(a1), "c"(a2) : "memory");
    return ret;
}

static inline i32 syscall3(u32 no, u32 a1, u32 a2, u32 a3) {
    i32 ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(no), "b"(a1), "c"(a2), "d"(a3) : "memory");
    return ret;
}

static i32 sysop_call(u32 op, u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    sysop_msg_s req = (sysop_msg_s){
        .arg0 = arg0,
        .arg1 = arg1,
        .arg2 = arg2,
        .arg3 = arg3
    };
    
    ipc_msg_s msg = (ipc_msg_s){
        .id = op,
        .sz = (u32)sizeof(sysop_msg_s)
    };
    
    __builtin_memcpy(msg.data, &req, sizeof(req));
    return syscall2(SYS_ipc_call, KERNEL_TASK_ID, (u32)&msg);
}

i32 sys_ipc_send(u32 dst, const ipc_msg_s* msg) {
    return syscall2(SYS_ipc_send, dst, (u32)msg);
}

i32 sys_ipc_recv(ipc_msg_s* out) {
    return syscall1(SYS_ipc_recv, (u32)out);
}

i32 sys_ipc_call(u32 dst, ipc_msg_s* msg) {
    return syscall2(SYS_ipc_call, dst, (u32)msg);
}

i32 sys_ipc_reply(u32 client, const ipc_msg_s* msg) {
    return syscall2(SYS_ipc_reply, client, (u32)msg);
}

i32 sys_thread_create(void (*entry)(void), u32 priority) {
    return syscall2(SYS_thread_create, (u32)entry, priority);
}

i32 sys_thread_yield(void) {
    return syscall0(SYS_thread_yield);
}

void sys_thread_exit(i32 code) {
    (void)syscall1(SYS_thread_exit, (u32)code);
    __builtin_unreachable();
}

i32 sys_map_pg(phys_addr_t paddr, virt_addr_t vaddr, u32 flags) {
    return syscall3(SYS_map_pg, (u32)vaddr, (u32)paddr, flags);
}

i32 sys_unmap_pg(const void* vaddr) {
    return syscall1(SYS_unmap_pg, (u32)vaddr);
}

i32 sys_log_read(void* dst, u32 len, u32 off) {
    return sysop_call(SYSOP_log_read, (u32)dst, len, off, 0);
}

i32 sys_server_lookup(const char* name) {
    const size_t len = strlen(name);

    if (len >= SERVER_MAX_NAME_LEN)
        return -(i32)E_INVAL;

    return sysop_call(SYSOP_server_lookup, (u32)name, (u32)(len + 1), 0, 0);
}

i32 sys_irq_register_handler(u32 irq) {
    return sysop_call(SYSOP_irq_register_handler, irq, 0, 0, 0);
}

i32 sys_irq_wait(u32 irq) {
    return sysop_call(SYSOP_irq_wait, irq, 0, 0, 0);
}