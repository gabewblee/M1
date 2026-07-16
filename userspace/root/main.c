#include <bootinfo.h>
#include <elf.h>
#include <mm.h>
#include <stdbool.h>
#include <uapi/capability.h>
#include <uapi/elf.h>
#include <uapi/errno.h>
#include <uapi/servers.h>
#include <userspace/libc/capability.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

#define ROOT_IMG_BASE     0x40000000u
#define ROOT_CP_BASE      0x40400000u
#define ROOT_IMG_MAX_PG   1024u
#define SERVER_STACK_TOP  0xBFFFF000u
#define SERVER_STACK_PG   4u
#define PG_DIR_IDX(vaddr) ((vaddr) >> 22)
#define PG_DIR_CNT        1024u
#define HIGHER_HALF_IDX   768u

typedef struct modinfo_s {
    u32 cnode;  /* CNode root slot    */
    u32 vspace; /* VSpace root slot   */
    u32 ep;     /* Endpoint root slot */
    u32 badge;  /* Server badge       */
} modinfo_s;

static const bootinfo_s* bi;
static u32               nxt; /* Next usable slot              */
static u32               end; /* One past the last usable slot */
static modinfo_s         infos[BOOTINFO_MAX_MOD_CNT];

static u32 cstrlen(const char* s) {
    u32 n;
    for (n = 0; s[n]; n++);
    return n;
}

static void log(const char* s) {
    sys_dbg_puts(s, cstrlen(s));
}

static void __noreturn fatal(const char* s) {
    log("[ROOT] FATAL: ");
    log(s);
    log("\n");
    sys_thread_exit(-(i32)E_FAULT);
    while (1);
}

static u32 alloc_capability_slot(void) {
    if (nxt >= end)
        fatal("Failed to allocate capability slot");

    return nxt++;
}

static u32 mk_obj_from(u32 untyped, u32 type, u32 nbits) {
    u32 slot = alloc_capability_slot();
    if (capability_retype(untyped, type, nbits, BOOT_SLOT_CNODE, ROOT_CNODE_RADIX, slot, 1) != E_OK)
        fatal("Failed to retype capability");

    return slot;
}

static u32 mk_obj(u32 type, u32 nbits) {
    return mk_obj_from(BOOT_SLOT_RAM, type, nbits);
}

static u32 server_cptr(u32 cnode_slot, u32 inner) {
    return (cnode_slot << SERVICE_CNODE_RADIX) | inner;
}

static u32 server_depth(void) {
    return ROOT_CNODE_RADIX + SERVICE_CNODE_DEPTH;
}

static void mint_capability_into(u32 src_slot, u32 cnode_slot, u32 inner, u32 badge) {
    bool ok = capability_mint(
        BOOT_SLOT_CNODE,
        server_cptr(cnode_slot, inner),
        server_depth(),
        src_slot,
        ROOT_CNODE_RADIX,
        CAPABILITY_RIGHTS_ALL,
        badge
    ) == E_OK;

    if (!ok)
        fatal("Failed to mint capability");
}

static void ensure_pg_table(u32 vspace, u8* mapped, u32 vaddr) {
    u32 index = PG_DIR_IDX(vaddr);
    if (index >= HIGHER_HALF_IDX || mapped[index])
        return;

    u32 table = mk_obj(CAPABILITY_TYPE_PG_TABLE, 0);
    if (map_pg_table(table, vspace, ROOT_CNODE_RADIX, index << 22) != E_OK)
        fatal("Failed to map page table");

    mapped[index] = 1;
}

static const u8* map_img_pgs(u32 first, u32 cnt) {
    if (cnt > ROOT_IMG_MAX_PG)
        fatal("Invalid server image size");

    for (u32 i = 0; i < cnt; i++)
        if (map_pg(first + i, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_IMG_BASE + i * PG_SZ, 0) != E_OK)
            fatal("Failed to map page");

    return (const u8*)ROOT_IMG_BASE;
}

static void unmap_img_pgs(u32 first, u32 cnt) {
    for (u32 i = 0; i < cnt; i++)
        unmap_pg(first + i, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_IMG_BASE + i * PG_SZ);
}

static void load_pg(u32 frm, u32 vspace, const u8* image, const elf32_phdr_s* ph, u32 vaddr) {
    u32 start = max(vaddr, ph->p_vaddr), end = min(vaddr + PG_SZ, ph->p_vaddr + ph->p_filesz);
    if (end > start) {
        if (map_pg(frm, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_CP_BASE, PG_RW_FLAG) != E_OK)
            fatal("Failed to map page");

        memcpy(
            (void*)(ROOT_CP_BASE + (start - vaddr)),
            (void*)(image + ph->p_offset + (start - ph->p_vaddr)),
            end - start
        );

        unmap_pg(frm, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_CP_BASE);
    }

    u32 flags = (ph->p_flags & PF_W) ? PG_RW_FLAG : 0u;
    if (map_pg(frm, vspace, ROOT_CNODE_RADIX, vaddr, flags) != E_OK)
        fatal("Failed to map page");
}

static u32 load_elf(const u8* image, u32 vspace) {
    const elf32_ehdr_s* eh = (const elf32_ehdr_s*)image;
    if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
        eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 ||
        eh->e_ident[EI_MAG3] != ELFMAG3)
        fatal("Invalid ELF magic");

    if (eh->e_ident[EI_CLASS] != ELFCLASS32 || eh->e_type != ET_EXEC || eh->e_machine != EM_386)
        fatal("Invalid ELF file");

    const elf32_phdr_s* ph = (const elf32_phdr_s*)(image + eh->e_phoff);
    static u8 mapped[PG_DIR_CNT];
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
            u32 frm = mk_obj(CAPABILITY_TYPE_FRM, 0);
            load_pg(frm, vspace, image, &ph[i], v);
        }
    }

    return eh->e_entry;
}

static void provision(u32 i) {
    const mod_s* mod = &bi->mods[i];
    modinfo_s* info = &infos[i];
    info->cnode     = mk_obj(CAPABILITY_TYPE_CNODE, SERVICE_CNODE_RADIX);
    info->vspace    = mk_obj(CAPABILITY_TYPE_PG_DIR, 0);
    info->badge     = mod->desc.id + 1u;
    info->ep        = mk_obj(CAPABILITY_TYPE_ENDPOINT, 0);
    mint_capability_into(info->ep, info->cnode, SERVICE_CPTR_EP, info->badge);

    u32 reply_slot = mk_obj(CAPABILITY_TYPE_REPLY, 0);
    mint_capability_into(reply_slot, info->cnode, SERVICE_CPTR_REPLY, 0);
    mint_capability_into(info->vspace, info->cnode, SERVICE_CPTR_VSPACE, 0);
}

static i32 find_ctx(u32 server_id) {
    for (u32 i = 0; i < bi->nmods; i++)
        if (bi->mods[i].desc.id == server_id)
            return (i32)i;

    return -1;
}

static void wire(u32 i) {
    const mod_s* mod = &bi->mods[i];
    modinfo_s* info = &infos[i];
    for (u32 r = 0; r < SERVER_MAX_RES_CNT && mod->desc.res[r].type != RESOURCE_TYPE_NONE; r++) {
        const resource_s* res = &mod->desc.res[r];
        switch (res->type) {
            case RESOURCE_TYPE_IRQ:
                if (irq_ctrl_get(BOOT_SLOT_IRQ_CTRL, res->arg, info->cnode, ROOT_CNODE_RADIX, res->slot) != E_OK)
                    fatal("Failed to get IRQ");
                break;

            case RESOURCE_TYPE_NTFN:
                mint_capability_into(mk_obj(CAPABILITY_TYPE_NTFN, 0), info->cnode, res->slot, 0);
                break;

            case RESOURCE_TYPE_DEV_FRM:
                mint_capability_into(mk_obj_from(res->arg, CAPABILITY_TYPE_FRM, 0), info->cnode, res->slot, 0);
                break;

            case RESOURCE_TYPE_SERV_EP: {
                i32 target = find_ctx(res->arg);
                if (target < 0)
                    fatal("Unknown server dependency");

                mint_capability_into(infos[target].ep, info->cnode, res->slot, info->badge);
                break;
            }

            default:
                fatal("Unknown resource type");
        }
    }
}

static void start(u32 i) {
    const mod_s* mod = &bi->mods[i];
    modinfo_s* info = &infos[i];

    const u8* img = map_img_pgs(mod->frm, mod->nfrms);
    u32 entry = load_elf(img, info->vspace);
    unmap_img_pgs(mod->frm, mod->nfrms);
    for (u32 s = 0; s < SERVER_STACK_PG; s++) {
        u32 frm = mk_obj(CAPABILITY_TYPE_FRM, 0);
        if (map_pg(frm, info->vspace, ROOT_CNODE_RADIX, SERVER_STACK_TOP - (s + 1) * PG_SZ, PG_RW_FLAG) != E_OK)
            fatal("Failed to map page");
    }

    u32 tcb = mk_obj(CAPABILITY_TYPE_TCB, 0);
    if (tcb_configure(
            tcb,
            info->cnode,
            ROOT_CNODE_RADIX,
            info->vspace,
            ROOT_CNODE_RADIX,
            entry,
            SERVER_STACK_TOP,
            mod->desc.priority
        ) != E_OK)
        fatal("Failed to configure TCB");

    if (tcb_resume(tcb) != E_OK)
        fatal("Failed to resume TCB");
}

int main(void) {
    bi = (const bootinfo_s*)BOOTINFO_VADDR;
    if (bi->magic != BOOTINFO_MAGIC)
        fatal("Invalid bootinfo magic");

    nxt = bi->empty;
    end = bi->empty + bi->nempty;

    log("[ROOT] Initializing root server\n");
    u32 img_table = mk_obj(CAPABILITY_TYPE_PG_TABLE, 0);
    if (map_pg_table(img_table, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_IMG_BASE) != E_OK)
        fatal("Failed to map image table");

    u32 cp_table = mk_obj(CAPABILITY_TYPE_PG_TABLE, 0);
    if (map_pg_table(cp_table, BOOT_SLOT_VSPACE, ROOT_CNODE_RADIX, ROOT_CP_BASE) != E_OK)
        fatal("Failed to map copy table");

    for (u32 i = 0; i < bi->nmods; i++)
        provision(i);

    for (u32 i = 0; i < bi->nmods; i++)
        wire(i);

    for (u32 i = 0; i < bi->nmods; i++)
        start(i);

    log("[ROOT] Initialized all servers\n");
    return E_OK;
}
