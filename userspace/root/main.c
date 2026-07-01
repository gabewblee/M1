#include <bootinfo.h>
#include <mm.h>
#include <uapi/capability.h>
#include <uapi/errno.h>
#include <uapi/servers.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/root/elf.h>

/* The root task is the system's root server. The kernel hands it a CSpace
   holding untyped memory, an IRQ control capability, its own VSpace, the VGA
   device frame, and one Frame capability per page of every other server's ELF
   image. From these it retypes the objects each server needs, loads the server
   images into freshly retyped frames, builds each server's address space and
   CSpace, and starts it - the userspace counterpart of seL4's CapDL loader. */

#define ROOT_IMG_BASE     0x40000000u /* Window where a server image is mapped     */
#define ROOT_COPY_BASE    0x40400000u /* Window where a destination frame is mapped */
#define ROOT_IMG_MAX_PG   1024u       /* Image window capacity in pages            */
#define SERVER_STACK_TOP  0xBFFFF000u /* Top of a server's user stack              */
#define SERVER_STACK_PG   4u          /* Server user stack page count              */
#define PD_INDEX(vaddr)   ((vaddr) >> 22)
#define PD_COUNT          1024u
#define HIGHER_HALF_INDEX 768u        /* First page directory index of the kernel  */

static const user_bootinfo_s* bi;
static u32                     next_slot; /* Root CNode slot allocator cursor */
static u32                     slot_end;  /* One past the last usable slot    */

static u32 cstrlen(const char* s) {
    u32 n = 0;
    while (s[n])
        n++;

    return n;
}

static void log(const char* s) {
    sys_dbg_puts(s, cstrlen(s));
}

static void __attribute__((__noreturn__)) fatal(const char* s) {
    log("[root] FATAL: ");
    log(s);
    log("\n");
    sys_thread_exit(-(i32)E_FAULT);
    for (;;) {}
}

static u32 alloc_slot(void) {
    if (next_slot >= slot_end)
        fatal("out of root CNode slots");

    return next_slot++;
}

/* Retypes a single object of @type out of the root untyped into a fresh root
   CNode slot and returns the slot's capability pointer. */
static u32 make_object(u32 type, u32 size_bits) {
    u32 slot = alloc_slot();
    if (capability_retype(bi->untyped, type, size_bits, bi->slot_cnode, bi->cnode_radix, slot, 1) != E_OK)
        fatal("retype failed");

    return slot;
}

/* Composes a cptr that resolves, from the root CSpace, into slot @inner of the
   server CNode held at root slot @cnode_slot. The root CNode consumes its radix
   bits first, then the server CNode consumes its own. */
static u32 server_cptr(u32 cnode_slot, u32 inner) {
    return (cnode_slot << SERVICE_CNODE_RADIX) | inner;
}

static u32 server_depth(void) {
    return bi->cnode_radix + SERVICE_CNODE_DEPTH;
}

static void mint_into(u32 src_slot, u32 cnode_slot, u32 inner, u32 badge) {
    if (capability_mint(bi->slot_cnode,
                        server_cptr(cnode_slot, inner), server_depth(),
                        src_slot, bi->cnode_radix,
                        CAPABILITY_RIGHTS_ALL, badge) != E_OK)
        fatal("mint failed");
}

/* Installs a page table for the 4 MiB region containing @vaddr in the VSpace at
   @vspace, unless one is already recorded in @mapped. */
static void ensure_pg_table(u32 vspace, u8* mapped, u32 vaddr) {
    u32 index = PD_INDEX(vaddr);
    if (index >= HIGHER_HALF_INDEX || mapped[index])
        return;

    u32 table = make_object(CAPABILITY_TYPE_PG_TABLE, 0);
    if (page_table_map(table, vspace, bi->cnode_radix, index << 22) != E_OK)
        fatal("page table map failed");

    mapped[index] = 1;
}

/* Maps the @count image frames starting at cptr @first read-only into the root
   task's own image window, returning a pointer to the start of the image. */
static const u8* map_image(u32 first, u32 count) {
    if (count > ROOT_IMG_MAX_PG)
        fatal("server image too large");

    for (u32 i = 0; i < count; i++)
        if (page_map(first + i, bi->slot_vspace, bi->cnode_radix, ROOT_IMG_BASE + i * PG_SZ, 0) != E_OK)
            fatal("image map failed");

    return (const u8*)ROOT_IMG_BASE;
}

static void unmap_image(u32 first, u32 count) {
    for (u32 i = 0; i < count; i++)
        page_unmap(first + i, bi->slot_vspace, bi->cnode_radix, ROOT_IMG_BASE + i * PG_SZ);
}

static u32 min_u32(u32 a, u32 b) { return a < b ? a : b; }
static u32 max_u32(u32 a, u32 b) { return a > b ? a : b; }

/* Copies one page of a loadable segment into the freshly retyped frame @frame,
   filling file-backed bytes from the image and leaving the rest zero, then maps
   the frame into the server VSpace @vspace at @vaddr. */
static void load_page(u32 frame, u32 vspace, const u8* image, const elf32_phdr_s* ph, u32 vaddr) {
    u32 copy_start = max_u32(vaddr, ph->p_vaddr);
    u32 copy_end   = min_u32(vaddr + PG_SZ, ph->p_vaddr + ph->p_filesz);

    if (copy_end > copy_start) {
        if (page_map(frame, bi->slot_vspace, bi->cnode_radix, ROOT_COPY_BASE, PG_RW_FLAG) != E_OK)
            fatal("copy window map failed");

        memcpy((void*)(ROOT_COPY_BASE + (copy_start - vaddr)),
               (void*)(image + ph->p_offset + (copy_start - ph->p_vaddr)),
               copy_end - copy_start);

        page_unmap(frame, bi->slot_vspace, bi->cnode_radix, ROOT_COPY_BASE);
    }

    u32 flags = (ph->p_flags & PF_W) ? PG_RW_FLAG : 0u;
    if (page_map(frame, vspace, bi->cnode_radix, vaddr, flags) != E_OK)
        fatal("segment map failed");
}

/* Loads the ELF image at @image into the VSpace at @vspace, returning the entry
   point. Reserves page tables for every loaded region and the stack, then
   retypes and fills a frame for each page of each loadable segment. */
static u32 load_elf(const u8* image, u32 vspace) {
    const elf32_ehdr_s* eh = (const elf32_ehdr_s*)image;
    if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3)
        fatal("bad ELF magic");

    if (eh->e_ident[EI_CLASS] != ELFCLASS32 || eh->e_type != ET_EXEC || eh->e_machine != EM_386)
        fatal("unsupported ELF");

    const elf32_phdr_s* ph = (const elf32_phdr_s*)(image + eh->e_phoff);

    static u8 mapped[PD_COUNT];
    memset(mapped, 0, sizeof(mapped));

    for (u16 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        u32 lo = ph[i].p_vaddr & ~(PG_SZ - 1u);
        u32 hi = (ph[i].p_vaddr + ph[i].p_memsz + PG_SZ - 1u) & ~(PG_SZ - 1u);
        for (u32 v = lo; v < hi; v += PG_SZ)
            ensure_pg_table(vspace, mapped, v);
    }

    for (u32 i = 0; i < SERVER_STACK_PG; i++)
        ensure_pg_table(vspace, mapped, SERVER_STACK_TOP - (i + 1) * PG_SZ);

    for (u16 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        u32 lo = ph[i].p_vaddr & ~(PG_SZ - 1u);
        u32 hi = (ph[i].p_vaddr + ph[i].p_memsz + PG_SZ - 1u) & ~(PG_SZ - 1u);
        for (u32 v = lo; v < hi; v += PG_SZ) {
            u32 frame = make_object(CAPABILITY_TYPE_FRM, 0);
            load_page(frame, vspace, image, &ph[i], v);
        }
    }

    return eh->e_entry;
}

/* Builds, loads, and starts one server, returning the root slot holding its
   service endpoint so clients can be wired to it. */
static u32 load_server(const bootinfo_module_s* mod, u32 vga_ep_slot) {
    u32 cnode  = make_object(CAPABILITY_TYPE_CNODE, SERVICE_CNODE_RADIX);
    u32 vspace = make_object(CAPABILITY_TYPE_PG_DIR, 0);

    const u8* image = map_image(mod->frame_first, mod->frame_count);
    u32       entry = load_elf(image, vspace);
    unmap_image(mod->frame_first, mod->frame_count);

    for (u32 i = 0; i < SERVER_STACK_PG; i++) {
        u32 frame = make_object(CAPABILITY_TYPE_FRM, 0);
        if (page_map(frame, vspace, bi->cnode_radix, SERVER_STACK_TOP - (i + 1) * PG_SZ, PG_RW_FLAG) != E_OK)
            fatal("stack map failed");
    }

    u32 badge = mod->server_id + 1u;

    u32 ep_slot = make_object(CAPABILITY_TYPE_ENDPOINT, 0);
    mint_into(ep_slot, cnode, SERVICE_CPTR_EP, badge);

    u32 reply_slot = make_object(CAPABILITY_TYPE_REPLY, 0);
    mint_into(reply_slot, cnode, SERVICE_CPTR_REPLY, 0);

    mint_into(vspace, cnode, SERVICE_CPTR_VSPACE, 0);

    if (mod->server_id == SERVER_ID_vga)
        mint_into(bi->vga_fb_frame, cnode, SERVICE_CPTR_VGA_FB, 0);
    else if (vga_ep_slot)
        mint_into(vga_ep_slot, cnode, SERVICE_CPTR_VGA, badge);

    if (mod->server_id == SERVER_ID_keyboard || mod->server_id == SERVER_ID_ata) {
        u32 ntfn = make_object(CAPABILITY_TYPE_NOTIFICATION, 0);
        mint_into(ntfn, cnode, SERVICE_CPTR_NTFN, 0);
    }

    if (mod->server_id == SERVER_ID_keyboard) {
        if (irq_control_get(bi->slot_irq_control, 1, cnode, bi->cnode_radix, SERVICE_CPTR_IRQ) != E_OK)
            fatal("irq get failed");
    } else if (mod->server_id == SERVER_ID_ata) {
        if (irq_control_get(bi->slot_irq_control, 14, cnode, bi->cnode_radix, SERVICE_CPTR_IRQ) != E_OK ||
            irq_control_get(bi->slot_irq_control, 15, cnode, bi->cnode_radix, SERVICE_CPTR_IRQ2) != E_OK)
            fatal("irq get failed");
    }

    u32 tcb = make_object(CAPABILITY_TYPE_TCB, 0);
    if (tcb_configure(tcb, cnode, bi->cnode_radix, vspace, bi->cnode_radix,
                      entry, SERVER_STACK_TOP, mod->priority) != E_OK)
        fatal("tcb configure failed");

    if (tcb_resume(tcb) != E_OK)
        fatal("tcb resume failed");

    return ep_slot;
}

static const bootinfo_module_s* find_module(u32 server_id) {
    for (u32 i = 0; i < bi->module_cnt; i++)
        if (bi->modules[i].server_id == server_id)
            return &bi->modules[i];

    return 0;
}

int main(void) {
    bi = (const user_bootinfo_s*)USER_BOOTINFO_VADDR;
    if (bi->magic != USER_BOOTINFO_MAGIC)
        fatal("bad bootinfo magic");

    next_slot = bi->empty_first;
    slot_end  = bi->empty_first + bi->empty_cnt;

    log("[root] root server starting\n");

    /* Scratch page tables backing the image and copy windows in the root's own
       address space. */
    u32 img_table = make_object(CAPABILITY_TYPE_PG_TABLE, 0);
    if (page_table_map(img_table, bi->slot_vspace, bi->cnode_radix, ROOT_IMG_BASE) != E_OK)
        fatal("image window table map failed");

    u32 copy_table = make_object(CAPABILITY_TYPE_PG_TABLE, 0);
    if (page_table_map(copy_table, bi->slot_vspace, bi->cnode_radix, ROOT_COPY_BASE) != E_OK)
        fatal("copy window table map failed");

    /* The VGA server starts first so its endpoint exists before clients that
       print through it are wired up and resumed. */
    u32 vga_ep_slot = 0;
    const bootinfo_module_s* vga = find_module(SERVER_ID_vga);
    if (vga)
        vga_ep_slot = load_server(vga, 0);

    for (u32 i = 0; i < bi->module_cnt; i++)
        if (bi->modules[i].server_id != SERVER_ID_vga)
            load_server(&bi->modules[i], vga_ep_slot);

    log("[root] all servers started\n");
    return E_OK;
}
