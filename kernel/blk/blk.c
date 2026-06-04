#include "config.h"
#include "kernel/blk/blk.h"
#include "libk/string.h"
#include "uapi/compiler.h"
#include "uapi/errno.h"

static blk_dev_s* devs[BLK_DEV_MAX_CNT];

i32 blk_register_dev(blk_dev_s* dev) {
    for (u32 i = 0; i < BLK_DEV_MAX_CNT; i++) {
        if (!devs[i]) {
            devs[i] = dev;
            return E_OK;
        }
    }
    return -(i32)E_NOMEM;
}

void blk_unregister_dev(const blk_dev_s* dev) {
    for (u32 i = 0; i < BLK_DEV_MAX_CNT; i++) {
        if (devs[i] == dev) {
            devs[i] = NULL;
            return;
        }
    }
}

blk_dev_s* blk_dev_lookup(const char* name) {
    for (u32 i = 0; i < BLK_DEV_MAX_CNT; i++) {
        if (devs[i] && !strcmp(devs[i]->name, name))
            return devs[i];
    }
    return NULL;
}

i32 blk_read(blk_dev_s* dev, u64 lba, u8 cnt, void* buf) {
    return dev->ops->read(dev, lba, cnt, buf);
}

i32 blk_write(blk_dev_s* dev, u64 lba, u8 cnt, const void* buf) {
    return dev->ops->write(dev, lba, cnt, buf);
}

i32 blk_info(blk_dev_s* dev, blk_dev_info_s* out) {
    return dev->ops->info(dev, out);
}

void __init blk_init(void) {
    for (u32 i = 0; i < BLK_DEV_MAX_CNT; i++)
        devs[i] = NULL;
}
