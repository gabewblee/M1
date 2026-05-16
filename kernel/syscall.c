#include "syscall.h"
#include "task.h"

#include "../mm/pmm.h"
#include "../mm/vmm.h"

#define _syscalls(X)        \
    X(0,  ipc_send,      3) \
    X(1,  ipc_recv,      2) \
    X(2,  ipc_call,      4) \
    X(3,  thread_create, 2) \
    X(4,  thread_yield,  0) \
    X(5,  thread_sleep,  1) \
    X(6,  thread_exit,   1) \
    X(7,  alloc_frms,    1) \
    X(8,  free_frms,     2) \
    X(9,  map_pg,        3) \
    X(10, unmap_pg,      1) \
    X(11, max,           0) 

#define _syscall_no_enum(id, name, argc) SYS_##name = (id),
typedef enum syscall_no_t {
    _syscalls(_syscall_no_enum)
} syscall_no_t;
#undef _syscall_no_enum

#define _syscall_packed(id, name, argc) _Static_assert((SYS_##name) == (id), "Error: '" #name "' is not at position " #id);
_syscalls(_syscall_packed)
#undef _syscall_packed

_Static_assert(SYS_max < 32, "Error: Too many syscalls defined");

#define _syscall_decl_0(name)             static i32 sys_##name(void)
#define _syscall_decl_1(name)             static i32 sys_##name(u32)
#define _syscall_decl_2(name)             static i32 sys_##name(u32, u32)
#define _syscall_decl_3(name)             static i32 sys_##name(u32, u32, u32)
#define _syscall_decl_4(name)             static i32 sys_##name(u32, u32, u32, u32)
#define _syscall_decl_5(name)             static i32 sys_##name(u32, u32, u32, u32, u32)
#define _syscall_fwd_decl(id, name, argc) _syscall_decl_##argc(name);
_syscalls(_syscall_fwd_decl)
#undef _syscall_fwd_decl

#define _syscall_args_0(frm)
#define _syscall_args_1(frm) (frm)->ebx
#define _syscall_args_2(frm) (frm)->ebx, (frm)->ecx
#define _syscall_args_3(frm) (frm)->ebx, (frm)->ecx, (frm)->edx
#define _syscall_args_4(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi
#define _syscall_args_5(frm) (frm)->ebx, (frm)->ecx, (frm)->edx, (frm)->esi, (frm)->edi

static i32 sys_ipc_send(u32 dst, u32 m0, u32 m1) {
    (void)dst;
    (void)m0;
    (void)m1;
    return -(i32)E_NOSYS;
}

static i32 sys_ipc_recv(u32 src, u32 unused) {
    (void)src;
    (void)unused;
    return -(i32)E_NOSYS;
}

static i32 sys_ipc_call(u32 dst, u32 m0, u32 m1, u32 m2) {
    (void)dst;
    (void)m0;
    (void)m1;
    (void)m2;
    return -(i32)E_NOSYS;
}

static i32 sys_thread_create(u32 entry, u32 arg) {
    (void)entry;
    (void)arg;
    return -(i32)E_NOSYS;
}

static i32 sys_thread_yield(void) {
    return -(i32)E_NOSYS;
}

static i32 sys_thread_sleep(u32 ms) {
    (void)ms;
    return -(i32)E_NOSYS;
}

static i32 sys_thread_exit(u32 code) {
    (void)code;
    return -(i32)E_NOSYS;
}

static i32 sys_alloc_frms(u32 count) {
    (void)count;
    return -(i32)E_NOSYS;
}

static i32 sys_free_frms(u32 paddr, u32 count) {
    (void)paddr;
    (void)count;
    return -(i32)E_NOSYS;
}

static i32 sys_map_pg(u32 paddr, u32 vaddr, u32 flags) {
    (void)paddr;
    (void)vaddr;
    (void)flags;
    return -(i32)E_NOSYS;
}

static i32 sys_unmap_pg(u32 vaddr) {
    (void)vaddr;
    return -(i32)E_NOSYS;
}

static i32 sys_max(void) {
    return -(i32)E_NOSYS;
}

void __hot syscall_handler(syscall_frm_t* frm) {
    static const void* const dispatch[SYS_max + 1] __aligned(64) = {
        #define _syscall_lbl(id, name, argc) [id] = &&l_##name,
        _syscalls(_syscall_lbl)
        #undef _syscall_lbl
    };

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