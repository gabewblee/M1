#pragma once

#include <uapi/uapi.h>

/**
 * blk_init - Probes drive @drive on the ATA server and records its geometry.
 * @drive: The ATA drive index.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_init(u32 drive);

/**
 * blk_sectors - Returns the capacity of the initialized drive in sectors.
 * Returns: The capacity of the initialized drive in sectors.
 */
u64 blk_sectors(void);

/**
 * blk_read - Reads @count sectors at @lba into @buffer.
 * @lba: The absolute sector address.
 * @count: The number of sectors to read.
 * @buffer: The destination buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_read(u64 lba, u32 count, void* buffer);

/**
 * blk_write - Writes @count sectors from @buffer at @lba.
 * @lba: The absolute sector address.
 * @count: The number of sectors to write.
 * @buffer: The source buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 blk_write(u64 lba, u32 count, const void* buffer);
