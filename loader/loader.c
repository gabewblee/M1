#include <stdbool.h>
#include <stddef.h>

#include "bits.h"
#include "config.h"
#include "elf.h"
#include "libk/string.h"
#include "loader/loader.h"
#include "mm/page.h"
#include "mm/vmm.h"
#include "uapi/compiler.h"

static i32 is_valid_ehdr(const elf32_ehdr_s* ehdr, size_t sz) {
    if (unlikely(sz < sizeof(elf32_ehdr_s)))
        return -1;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0)
        return -1;

    if (ehdr->e_ident[EI_MAG1] != ELFMAG1)
        return -1;

    if (ehdr->e_ident[EI_MAG2] != ELFMAG2)
        return -1;

    if (ehdr->e_ident[EI_MAG3] != ELFMAG3)
        return -1;

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
        return -1;

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return -1;

    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
        return -1;

    if (ehdr->e_version != EV_CURRENT)
        return -1;

    if (ehdr->e_type != ET_EXEC)
        return -1;

    if (ehdr->e_machine != EM_386)
        return -1;

    if (ehdr->e_phentsize != sizeof(elf32_phdr_s))
        return -1;

    if (ehdr->e_phnum == 0)
        return -1;

    if (ehdr->e_entry == 0 || ehdr->e_entry >= HIGHER_HALF_OFFSET)
        return -1;

    const u64 end = (u64)ehdr->e_phoff + (u64)ehdr->e_phnum * sizeof(elf32_phdr_s);
    if (end > sz)
        return -1;

    return 0;
}

static i32 is_valid_load_seg(const elf32_phdr_s* phdr, size_t sz) {
    if (phdr->p_vaddr == 0)
        return -1;

    if ((u64)phdr->p_offset + (u64)phdr->p_filesz > sz)
        return -1;

    if ((u64)phdr->p_vaddr + (u64)phdr->p_memsz > HIGHER_HALF_OFFSET)
        return -1;

    if ((phdr->p_vaddr & (PG_SZ - 1)) != (phdr->p_offset & (PG_SZ - 1)))
        return -1;

    return 0;
}

static u32 pg_flags_at(const elf32_phdr_s* phdr, const u16 phdr_cnt, virt_addr_t vaddr) {
    u32 flags = PG_USER_FLAG;
    for (u16 i = 0; i < phdr_cnt; i++) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        virt_addr_t lo = ALIGN_DOWN_TO(phdr[i].p_vaddr, PG_SZ);
        virt_addr_t hi = ALIGN_UP_TO(phdr[i].p_vaddr + phdr[i].p_memsz, PG_SZ);
        if (vaddr >= lo && vaddr < hi && (phdr[i].p_flags & PF_W))
            flags |= PG_RW_FLAG;
    }
    return flags;
}

virt_addr_t load_elf_from_memory(const void* data, size_t sz) {
    const elf32_ehdr_s* ehdr = (const elf32_ehdr_s*)data;
    if (unlikely(is_valid_ehdr(ehdr, sz) == -1))
        return 0;

    const elf32_phdr_s* phdr = (const elf32_phdr_s*)((const u8*)data + ehdr->e_phoff);
    const u16 phdr_cnt = ehdr->e_phnum;

    virt_addr_t prev = 0; bool ok = false;
    for (u16 i = 0; i < phdr_cnt; i++) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        if (unlikely(is_valid_load_seg(&phdr[i], sz) == -1))
            return 0;

        /* PT_LOAD entries must be sorted by p_vaddr */
        if (unlikely(phdr[i].p_vaddr < prev))
            return 0;

        prev = phdr[i].p_vaddr;
        if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz && (phdr[i].p_flags & PF_X))
            ok = true;
    }

    if (unlikely(!ok))
        return 0;

    virt_addr_t cursor = 0;
    for (u16 i = 0; i < phdr_cnt; i++) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        virt_addr_t lo = ALIGN_DOWN_TO(phdr[i].p_vaddr, PG_SZ);
        virt_addr_t hi = ALIGN_UP_TO(phdr[i].p_vaddr + phdr[i].p_memsz, PG_SZ);
        if (lo < cursor)
            lo = cursor;

        for (virt_addr_t vaddr = lo; vaddr < hi; vaddr += PG_SZ)
            vmm_demand_map(vaddr, PG_USER_FLAG | PG_RW_FLAG);

        if (hi > cursor)
            cursor = hi;
    }

    for (u16 i = 0; i < phdr_cnt; i++) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        memcpy((void*)phdr[i].p_vaddr, (const u8*)data + phdr[i].p_offset, phdr[i].p_filesz);
        if (phdr[i].p_memsz > phdr[i].p_filesz)
            memset((void*)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
    }

    for (u16 i = 0; i < phdr_cnt; i++) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        virt_addr_t lo = ALIGN_DOWN_TO(phdr[i].p_vaddr, PG_SZ);
        virt_addr_t hi = ALIGN_UP_TO(phdr[i].p_vaddr + phdr[i].p_memsz, PG_SZ);
        for (virt_addr_t vaddr = lo; vaddr < hi; vaddr += PG_SZ)
            vmm_set_pg_flags(vaddr, pg_flags_at(phdr, phdr_cnt, vaddr));
    }

    return ehdr->e_entry;
}
