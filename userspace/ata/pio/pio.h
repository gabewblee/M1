#pragma once

#include <userspace/ata/driver.h>

/**
 * ata_pio_read - Reads @cnt sectors from @lba into @buf in PIO mode.
 * @drv: The ATA drive metadata.
 * @lba: The absolute LBA address.
 * @cnt: The sector count.
 * @buf: The destination buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ata_pio_read(ata_drv_s* drv, u64 lba, u16 cnt, void* buf);

/**
 * ata_pio_write - Writes @cnt sectors from @buf into @lba in PIO mode.
 * @drv: The ATA drive metadata.
 * @lba: The absolute LBA address.
 * @cnt: The sector count.
 * @buf: The source buffer.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ata_pio_write(ata_drv_s* drv, u64 lba, u16 cnt, void* buf);