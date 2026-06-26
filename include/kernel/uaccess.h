#pragma once

#include <kernel/core/task.h>
#include <kernel/sync/spinlock.h>
#include <libk/string.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/errno.h>

/**
 * uaccess_range_ok - Checks that [@uaddr, @uaddr + @nbytes) is entirely within the user half.
 * @uaddr: The virtual address of the beginning of the range.
 * @nbytes: The range length.
 * Returns: true if the range is valid, false otherwise.
 */
static inline bool uaccess_range_ok(void* uaddr, size_t nbytes) {
    if (nbytes == 0)
        return true;
    
    virt_addr_t base = (virt_addr_t)uaddr;
    if (base + (virt_addr_t)nbytes < base)
        return false;

    return base + (virt_addr_t)nbytes <= HIGHER_HALF_OFFSET;
}

/**
 * copy_from_user - Copies @nbytes from user buffer @src into kernel buffer @dst.
 * @ucr3: The user page directory.
 * @dst: The kernel destination buffer.
 * @src: The user source buffer.
 * @nbytes: The number of bytes to copy.
 * Returns: E_OK on success, or -E_FAULT if @src is not readable.
 */
static i32 copy_from_user(phys_addr_t ucr3, void* dst, void* src, size_t nbytes) {
    if (!uaccess_range_ok(src, nbytes))
        return -(i32)E_FAULT;

    phys_addr_t saved;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");

    if (saved != ucr3)
        __asm__ volatile("mov %0, %%cr3" : : "r"(ucr3) : "memory");

    bool ok = vmm_range_accessible((virt_addr_t)src, nbytes, PG_USER_FLAG);
    if (ok) {
        memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);
        memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);
    }

    if (saved != ucr3)
        __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");

    return ok ? E_OK : -(i32)E_FAULT;
}

/**
 * copy_to_user - Copies @nbytes from kernel buffer @src into user buffer @dst.
 * @ucr3: The user page directory.
 * @dst: The user destination buffer.
 * @src: The kernel source buffer.
 * @nbytes: The number of bytes to copy.
 * Returns: E_OK on success, or -E_FAULT if @dst is not writable.
 */
static i32 copy_to_user(phys_addr_t ucr3, void* dst, void* src, size_t nbytes) {
    if (!uaccess_range_ok(dst, nbytes))
        return -(i32)E_FAULT;

    phys_addr_t saved;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");

    if (saved != ucr3)
        __asm__ volatile("mov %0, %%cr3" : : "r"(ucr3) : "memory");

    memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);
    bool ok = vmm_range_accessible((virt_addr_t)dst, nbytes, PG_USER_FLAG | PG_RW_FLAG);
    if (ok)
        memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);
        
    if (saved != ucr3)
        __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");

    return ok ? E_OK : -(i32)E_FAULT;
}

/**
 * copy_between_user - Copies @nbytes from user buffer @src in address space
 *                     @scr3 into user buffer @dst in address space @dcr3.
 * @dcr3: The destination page directory.
 * @dst: The destination user buffer.
 * @scr3: The source page directory.
 * @src: The source user buffer.
 * @nbytes: The number of bytes to copy.
 * Returns: E_OK on success, or -E_FAULT if either range is invalid.
 */
static i32 copy_between_user(phys_addr_t dcr3, void* dst, phys_addr_t scr3, void* src, size_t nbytes) {
    if (!uaccess_range_ok(src, nbytes) || !uaccess_range_ok(dst, nbytes))
        return -(i32)E_FAULT;

    phys_addr_t saved;
    __asm__ volatile("mov %%cr3, %0" : "=r"(saved) : : "memory");

    i32 ret = E_OK;
    for (size_t off = 0; off < nbytes;) {
        size_t chunk = nbytes - off;
        if (chunk > PG_SZ)
            chunk = PG_SZ;

        bool ok;
        {
            spin_guard(&vmm_scratch_lk);

            if (saved != scr3)
                __asm__ volatile("mov %0, %%cr3" : : "r"(scr3) : "memory");

            ok = vmm_range_accessible((virt_addr_t)((u8*)src + off), chunk, PG_USER_FLAG);
            if (ok)
                memcpy((void*)VMM_IPC_SCRATCH, (u8*)src + off, chunk);

            if (scr3 != dcr3)
                __asm__ volatile("mov %0, %%cr3" : : "r"(dcr3) : "memory");
                
            if (ok && (ok = vmm_range_accessible((virt_addr_t)((u8*)dst + off), chunk, PG_USER_FLAG | PG_RW_FLAG)))
                memcpy((u8*)dst + off, (void*)VMM_IPC_SCRATCH, chunk);

            if (saved != dcr3)
                __asm__ volatile("mov %0, %%cr3" : : "r"(saved) : "memory");
        }

        if (!ok) {
            ret = -(i32)E_FAULT;
            break;
        }

        off += chunk;
    }

    return ret;
}
