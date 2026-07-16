#pragma once

#include <types.h>

#define EI_MAG0        0      /* File identification    */
#define EI_MAG1        1      /* File identification    */
#define EI_MAG2        2      /* File identification    */
#define EI_MAG3        3      /* File identification    */
#define EI_CLASS       4      /* File class             */
#define EI_DATA        5      /* Data encoding          */
#define EI_VERSION     6      /* File version           */
#define EI_PAD         7      /* Start of padding bytes */
#define EI_NIDENT      16     /* Size of e_ident[]      */

#define ELFMAG0        0x7F   /* e_ident[EI_MAG0]       */
#define ELFMAG1        'E'    /* e_ident[EI_MAG1]	    */
#define ELFMAG2        'L'    /* e_ident[EI_MAG2]	    */
#define ELFMAG3        'F'    /* e_ident[EI_MAG3]	    */
#define ELFCLASSNONE   0      /* Invalid class          */
#define ELFCLASS32     1      /* 32-bit objects         */
#define ELFCLASS64     2      /* 64-bit objects         */
#define ELFDATANONE    0      /* Invalid data encoding  */
#define ELFDATA2LSB    1      /* Little-endian data     */
#define ELFDATA2MSB    2      /* Big-endian data        */

#define ET_NONE        0      /* No file type           */
#define ET_REL         1      /* Relocatable file       */
#define ET_EXEC        2      /* Executable file        */
#define ET_DYN         3      /* Shared object file     */
#define ET_CORE        4      /* Core file              */
#define ET_LOPROC      0xFF00 /* Processor-specific     */
#define ET_HIPROC      0xFFFF /* Processor-specific     */

#define EM_NONE        0      /* No machine             */
#define EM_M32         1      /* AT&T WE 32100          */
#define EM_SPARC       2      /* SPARC                  */
#define EM_386         3      /* Intel Architecture     */
#define EM_68K         4      /* Motorola 68000         */
#define EM_88K         5      /* Motorola 88000         */
#define EM_860         7      /* Intel 80860            */
#define EM_MIPS        8      /* MIPS RS3000 Big-Endian */
#define EM_MIPS_RS4_BE 10     /* MIPS RS4000 Big-Endian */
#define RESERVED1      11     /* Reserved               */
#define RESERVED2      12     /* Reserved               */
#define RESERVED3      13     /* Reserved               */
#define RESERVED4      14     /* Reserved               */
#define RESERVED5      15     /* Reserved               */
#define RESERVED6      16     /* Reserved               */

#define EV_NONE        0      /* Invalid version        */
#define EV_CURRENT     1      /* Current version        */

typedef u32 elf32_addr_t;
typedef u16 elf32_half_t;
typedef u32 elf32_off_t;
typedef i32 elf32_sword_t;
typedef u32 elf32_word_t;

typedef struct elf32_ehdr_s {
    u8           e_ident[EI_NIDENT]; /* ELF identification                */
    elf32_half_t e_type;             /* Object file type                  */
    elf32_half_t e_machine;          /* Required architecture             */
    elf32_word_t e_version;          /* Object file version               */
    elf32_addr_t e_entry;            /* Entry point                       */
    elf32_off_t  e_phoff;            /* Program header table file offset  */
    elf32_off_t  e_shoff;            /* Section header table file offset  */
    elf32_word_t e_flags;            /* Processor-specific flags          */
    elf32_half_t e_ehsize;           /* ELF header size                   */
    elf32_half_t e_phentsize;        /* Program header table entry size   */
    elf32_half_t e_phnum;            /* Program header table entry count  */
    elf32_half_t e_shentsize;        /* Section header table entry size   */
    elf32_half_t e_shnum;            /* Section header table entry count  */
    elf32_half_t e_shstrndx;         /* Section header string table index */
} elf32_ehdr_s;

#define PT_NULL	   0 		  /* Unused program header 		 */
#define PT_LOAD	   1 		  /* Loadable segment            */
#define PT_DYNAMIC 2 		  /* Dynamic linking information */
#define PT_INTERP  3 		  /* Program interpreter   		 */
#define PT_NOTE	   4 		  /* Auxiliary information 		 */
#define PT_SHLIB   5 		  /* Reserved  		 	   		 */
#define PT_PHDR	   6 		  /* Program header table  		 */
#define PT_LOPROC  0x70000000 /* Processor-specific    		 */
#define PT_HIPROC  0x7FFFFFFF /* Processor-specific    		 */

#define PF_X       0x1        /* Executable segment          */
#define PF_W       0x2        /* Writable segment            */
#define PF_R       0x4        /* Readable segment            */

typedef struct elf32_phdr_s {
	elf32_word_t p_type;   /* Segment type 		       */
	elf32_off_t  p_offset; /* Segment offset 		   */
	elf32_addr_t p_vaddr;  /* Segment virtual address  */
	elf32_addr_t p_paddr;  /* Segment physical address */
	elf32_word_t p_filesz; /* Segment size on disk     */
	elf32_word_t p_memsz;  /* Segment size in memory   */
	elf32_word_t p_flags;  /* Segment flags 		   */
	elf32_word_t p_align;  /* Segment alignment        */
} elf32_phdr_s;

typedef struct elf32_nhdr_s {
	elf32_word_t n_namesz; /* Note name size       */
	elf32_word_t n_descsz; /* Note descriptor size */
	elf32_word_t n_type;   /* Note type            */
} elf32_nhdr_s;
