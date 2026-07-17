#include <userspace/libc/heap.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/fsclient.h>
#include <userspace/vfs/super.h>

static const fstype_s fstab[] = {
    { "tmpfs", SERVICE_CPTR_TMPFS },
    { "ext2",  SERVICE_CPTR_EXT2  },
};

static cache_s rsb_object_cache;
static u32     next_device_number = 1;

const fstype_s* fstype_find(const char* name) {
    for (u32 i = 0; i < sizeof(fstab) / sizeof(fstab[0]); i++)
        if (strcmp(fstab[i].name, name) == 0)
            return &fstab[i];

    return NULL;
}

i32 rsb_mount(const fstype_s* fstype, const char* source, u32 flags, rsb_s** result) {
    rsb_s* sb = cache_alloc(&rsb_object_cache);
    if (!sb)
        return -(i32)E_NOMEM;

    fs_mount_reply_s reply;
    i32 ret = fsc_mount(fstype->ep, source, flags, &reply);
    if (ret != E_OK) {
        cache_free(&rsb_object_cache, sb);
        return ret;
    }

    sb->count     = 1;
    sb->dev       = next_device_number++;
    sb->sb        = reply.sb;
    sb->root_ino  = reply.root;
    sb->blocksize = reply.blocksize;
    sb->magic     = reply.magic;
    sb->fstype    = fstype;
    *result = sb;
    return E_OK;
}

rsb_s* rsb_get(rsb_s* sb) {
    sb->count++;
    return sb;
}

void rsb_put(rsb_s* sb) {
    if (--sb->count)
        return;

    fsc_umount(sb);
    cache_free(&rsb_object_cache, sb);
}

void super_init(void) {
    cache_init(&rsb_object_cache, "rsb", sizeof(rsb_s));
}
