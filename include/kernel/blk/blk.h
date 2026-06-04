#pragma once

#include "config.h"
#include "uapi/compiler.h"
#include "uapi/types.h"

#define BLK_DEV_SECTOR_SZ 512u /* Sector size */

typedef struct blk_dev_s blk_dev_s;

typedef struct blk_dev_info_s {
    u64 sectors; /* Sector count */
    u16 sz;      /* Sector size  */
} blk_dev_info_s;

typedef struct blk_dev_ops_s {
    i32 (*read)(blk_dev_s* dev, u64 lba, u8 cnt, void* buf);
    i32 (*write)(blk_dev_s* dev, u64 lba, u8 cnt, const void* buf);
    i32 (*info)(blk_dev_s* dev, blk_dev_info_s* out);
} blk_dev_ops_s;

typedef struct blk_dev_s {
    const char*          name; /* Device name         */
    const blk_dev_ops_s* ops;  /* Device operations   */
    void*                priv; /* Device-private data */
} blk_dev_s;

/**
 * blk_register_dev - Registers the block device @dev.
 * @dev: The block device to register.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_register_dev(blk_dev_s* dev);

/**
 * blk_unregister_dev - Unregisters the block device @dev.
 * @dev: The block device to unregister.
 */
void blk_unregister_dev(const blk_dev_s* dev);

/**
 * blk_dev_lookup - Looks up the block device by @name.
 * @name: The NULL-terminated device name.
 * Returns: The matching block device, or NULL on failure.
 */
blk_dev_s* blk_dev_lookup(const char* name);

/**
 * blk_read - Reads @cnt sectors into @buf from @dev starting at @lba.
 * @dev: The block device to read from.
 * @lba: The starting logical block address.
 * @cnt: The number of sectors to read.
 * @buf: The output buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_read(blk_dev_s* dev, u64 lba, u8 cnt, void* buf);

/**
 * blk_write - Writes @cnt sectors from @buf to @dev starting at @lba.
 * @dev: The block device to write to.
 * @lba: The starting logical block address.
 * @cnt: The number of sectors to write.
 * @buf: The input buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_write(blk_dev_s* dev, u64 lba, u8 cnt, const void* buf);

/**
 * blk_info - Returns information about @dev.
 * @dev: The block device to query.
 * @out: The output buffer for the device info.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_info(blk_dev_s* dev, blk_dev_info_s* out);

/**
 * blk_init - Initializes the block device subsystem.
 */
void __init blk_init(void);