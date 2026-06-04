#pragma once

#include <stdbool.h>

#include "uapi/ata.h"
#include "uapi/types.h"

typedef struct ata_channel_s {
    u16 io;     /* Base I/O port       */
    u16 ctrl;   /* Base control port   */
    u8  irq;    /* PIC IRQ number      */
    u8  select; /* Last selected drive */
} ata_channel_s;

typedef struct ata_drv_s {
    ata_channel_s* channel; /* Owning ATA channel                */
    u8             kind;    /* Drive kind                        */
    u8             slave;   /* 0 for master, 1 for slave         */
    u8             present; /* Whether the drive exists          */
    u8             lba48;   /* Whether LBA48 is supported        */
    u32            total28; /* Total sector count for LBA28 mode */
    u64            total48; /* Total sector count for LBA48 mode */
} ata_drv_s;

/**
 * ata_read_status - Reads the ATA status register.
 * @channel: The ATA channel.
 * Returns: The ATA status byte.
 */
u8 ata_read_status(const ata_channel_s* channel);

/**
 * ata_read_alt_status - Reads the ATA alternate status register.
 * @channel: The ATA channel.
 * Returns: The ATA alternate status byte.
 */
u8 ata_read_alt_status(const ata_channel_s* channel);

/**
 * ata_ready_wait - Waits until @channel is ready to send/receive data.
 * @channel: The ATA channel.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ata_ready_wait(const ata_channel_s* channel);

/**
 * ata_select_wait - Waits until the drive select command has settled.
 * @channel: The ATA channel.
 */
void ata_select_wait(const ata_channel_s* channel);

/**
 * ata_select_drv - Selects the master/slave drive on @channel.
 * @channel: The ATA channel.
 * @slave: The drive select; 0 for master, 1 for slave.
 */
void ata_select_drv(ata_channel_s* channel, u8 slave);

/**
 * ata_drv_lookup - Returns the drive metadata by index.
 * @idx: The drive index.
 * Returns: The drive metadata on success, or NULL on failure.
 */
ata_drv_s* ata_drv_lookup(u32 idx);

/**
 * ata_get_present_drv_cnt - Returns the number of present drives.
 * Returns: The number of present drives.
 */
u32 ata_get_present_drv_cnt(void);

/**
 * ata_drvs_init - Initializes the drives on both ATA channels.
 */
void ata_drvs_init(void);