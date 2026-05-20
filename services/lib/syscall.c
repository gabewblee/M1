#include "syscall.h"

static inline i32 syscall0(u32 no) {
    i32 ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(no) : "memory");
    return ret;
}

static inline i32 syscall1(u32 no, u32 a1) {
    i32 ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(no), "b"(a1) : "memory");
    return ret;
}

static inline i32 syscall2(u32 no, u32 a1, u32 a2) {
    i32 ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(no), "b"(a1), "c"(a2) : "memory");
    return ret;
}

static inline i32 syscall3(u32 no, u32 a1, u32 a2, u32 a3) {
    i32 ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(no), "b"(a1), "c"(a2), "d"(a3) : "memory");
    return ret;
}

static i32 sysop_call(sysop_no_t op, u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    ipc_msg_t msg = {0};
    sysop_msg_t req = {
        .arg0 = arg0,
        .arg1 = arg1,
        .arg2 = arg2,
        .arg3 = arg3
    };

    msg.id = (u32)op;
    msg.sz = (u32)sizeof(sysop_msg_t);

    __builtin_memcpy(msg.data, &req, sizeof(req));
    return syscall2(SYS_ipc_call, KERNEL_TASK_ID, (u32)&msg);
}

i32 sys_ipc_send(u32 dst, const ipc_msg_t* msg) {
    return syscall2(SYS_ipc_send, dst, (u32)msg);
}

i32 sys_ipc_recv(ipc_msg_t* msg) {
    return syscall1(SYS_ipc_recv, (u32)msg);
}

i32 sys_ipc_call(u32 dst, ipc_msg_t* msg) {
    return syscall2(SYS_ipc_call, dst, (u32)msg);
}

i32 sys_ipc_reply(u32 client, const ipc_msg_t* msg) {
    return syscall2(SYS_ipc_reply, client, (u32)msg);
}

i32 sys_thread_create(void (*entry)(void), u32 priority) {
    return sysop_call(SYSOP_thread_create, (u32)entry, priority, 0, 0);
}

i32 sys_thread_yield(void) {
    return sysop_call(SYSOP_thread_yield, 0, 0, 0, 0);
}

void sys_thread_exit(i32 code) {
    (void)sysop_call(SYSOP_thread_exit, (u32)code, 0, 0, 0);
    __builtin_unreachable();
}

i32 sys_log_read(void* dst, u32 len, u32 off) {
    return sysop_call(SYSOP_log_read, (u32)dst, len, off, 0);
}

i32 sys_map_pg(void* vaddr, phys_addr_t paddr, u32 flags) {
    return sysop_call(SYSOP_map_pg, (u32)vaddr, (u32)paddr, flags, 0);
}

i32 sys_unmap_pg(void* vaddr) {
    return sysop_call(SYSOP_unmap_pg, (u32)vaddr, 0, 0, 0);
}