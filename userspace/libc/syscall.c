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

static inline i32 syscall4(u32 no, u32 a1, u32 a2, u32 a3, u32 a4) {
    i32 ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(no), "b"(a1), "c"(a2), "d"(a3), "S"(a4)
        : "memory");
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


i32 sys_send(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr) {
    return syscall4(SYS_send, cptr, mi, (u32)packet, grant_cptr);
}

i32 sys_nbsend(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr) {
    return syscall4(SYS_nbsend, cptr, mi, (u32)packet, grant_cptr);
}

i32 sys_call(u32 cptr, msg_info_t mi, ipc_packet_s* packet, u32 grant_cptr) {
    return syscall4(SYS_call, cptr, mi, (u32)packet, grant_cptr);
}

i32 sys_recv(u32 cptr, u32 reply_cptr, ipc_packet_s* packet, u32* badge, u32 recv_slot) {
    return syscall5(SYS_recv, cptr, reply_cptr, (u32)packet, (u32)badge, recv_slot);
}

i32 sys_nbrecv(u32 cptr, u32 reply_cptr, ipc_packet_s* packet, u32* badge, u32 recv_slot) {
    return syscall5(SYS_nbrecv, cptr, reply_cptr, (u32)packet, (u32)badge, recv_slot);
}

i32 sys_reply(u32 reply_cptr, msg_info_t mi, ipc_packet_s* packet) {
    return syscall3(SYS_reply, reply_cptr, mi, (u32)packet);
}

i32 sys_replyrecv(u32 cptr, u32 reply_cptr, msg_info_t mi, ipc_packet_s* packet, u32* badge) {
    return syscall5(SYS_replyrecv, cptr, reply_cptr, mi, (u32)packet, (u32)badge);
}

i32 sys_signal(u32 cptr) {
    return syscall1(SYS_signal, cptr);
}

i32 sys_wait(u32 cptr, u32* badge) {
    return syscall2(SYS_wait, cptr, (u32)badge);
}

i32 sys_yield(void) {
    return syscall0(SYS_yield);
}

void sys_thread_exit(i32 code) {
    (void)syscall1(SYS_thread_exit, (u32)code);
    __builtin_unreachable();
}

i32 sys_dbg_puts(const char* str, u32 len) {
    return syscall2(SYS_dbg_puts, (u32)str, len);
}