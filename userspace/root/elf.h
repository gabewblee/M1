#pragma once

#include <uapi/types.h>

/* Minimal ELF32 definitions the root task needs to load server images. The
   userspace include path does not reach the kernel's <elf.h>, so the handful of
   structures and constants the loader uses are declared here. */

#define EI_NIDENT  16
#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6

#define ELFMAG0      0x7F
#define ELFMAG1      'E'
#define ELFMAG2      'L'
#define ELFMAG3      'F'
#define ELFCLASS32   1
#define ELFDATA2LSB  1
#define EV_CURRENT   1

#define ET_EXEC 2
#define EM_386  3

#define PT_LOAD 1

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct elf32_ehdr_s {
    u8  e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} elf32_ehdr_s;

typedef struct elf32_phdr_s {
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
} elf32_phdr_s;
