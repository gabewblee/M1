#pragma once

#include "core/task.h"
#include "libk/string.h"
#include "mm/vmm.h"

/**
 * copy_from_user - Copies @nbytes from user buffer @src into kernel buffer @dst.
 * @ucr3: The user page directory.
 * @dst: The kernel destination buffer.
 * @src: The user source buffer.
 * @nbytes: The number of bytes to copy.
 */
static void copy_from_user(const phys_addr_t ucr3, void* dst, const void* src, size_t nbytes) {
    phys_addr_t saved, kcr3 = task_lookup(0)->cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");

    __asm__ volatile("mov %0, %%cr3" : : "r"(ucr3) : "memory");
    memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(kcr3) : "memory");
    memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");
}

/**
 * copy_to_user - Copies @nbytes from kernel buffer @src into user buffer @dst.
 * @ucr3: The user page directory.
 * @dst: The user destination buffer.
 * @src: The kernel source buffer.
 * @nbytes: The number of bytes to copy.
 */
static void copy_to_user(const phys_addr_t ucr3, void* dst, const void* src, size_t nbytes) {
    phys_addr_t saved, kcr3 = task_lookup(0)->cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");

    __asm__ volatile("mov %0, %%cr3" : : "r"(kcr3) : "memory");
    memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(ucr3) : "memory");
    memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");
}
