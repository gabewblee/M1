#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

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

static inline i32 syscall5(u32 no, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5) {
    i32 ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(no), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
        : "memory");
    return ret;
}


i32 sys_ipc_send(u32 dst, ipc_packet_s* packet) {
    return syscall2(SYS_ipc_send, dst, (u32)packet);
}

i32 sys_ipc_recv(ipc_packet_s* packet) {
    return syscall1(SYS_ipc_recv, (u32)packet);
}

i32 sys_ipc_call(u32 dst, ipc_packet_s* packet) {
    return syscall2(SYS_ipc_call, dst, (u32)packet);
}

i32 sys_ipc_reply(u32 client, ipc_packet_s* packet) {
    return syscall2(SYS_ipc_reply, client, (u32)packet);
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

i32 sys_unmap_pg(void* vaddr) {
    return syscall1(SYS_unmap_pg, (u32)vaddr);
}

i32 sys_log_read(void* dst, u32 len, u32 off) {
    return syscall3(SYS_log_read, (u32)dst, len, off);
}

i32 sys_server_lookup(u32 id) {
    return syscall1(SYS_server_lookup, id);
}

i32 sys_irq_register_handler(u32 irq) {
    return syscall1(SYS_irq_register_handler, irq);
}

i32 sys_irq_wait_for(u32 irq) {
    return syscall1(SYS_irq_wait_for, irq);
}

i32 sys_vm_copy(u32 id, void* dst, void* src, u32 len, u32 dir) {
    return syscall5(SYS_vm_copy, id, (u32)dst, (u32)src, len, dir);
}