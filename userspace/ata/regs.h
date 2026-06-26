#pragma once

#include <uapi/uapi.h>
#include <userspace/ata/driver.h>

/* ATA base I/O port offsets */
#define ATA_DATA_REG_OFFSET       0u    /* Data register               */
#define ATA_ERROR_REG_OFFSET      1u    /* Error register              */
#define ATA_FEATURES_REG_OFFSET   1u    /* Features register           */
#define ATA_SECTOR_CNT_REG_OFFSET 2u    /* Sector count register       */
#define ATA_LBALO_REG_OFFSET      3u    /* LBA low register            */
#define ATA_LBAMID_REG_OFFSET     4u    /* LBA mid register            */
#define ATA_LBAHI_REG_OFFSET      5u    /* LBA high register           */
#define ATA_DRIVE_REG_OFFSET      6u    /* Drive register              */
#define ATA_STATUS_REG_OFFSET     7u    /* Status register             */
#define ATA_CMD_REG_OFFSET        7u    /* Command register            */

/* ATA status register bits  */
#define ATA_STATUS_REG_ERR_BIT    0x01u /* Error bit                   */
#define ATA_STATUS_REG_DRQ_BIT    0x08u /* Data send/receive bit       */
#define ATA_STATUS_REG_DF_BIT     0x20u /* Drive fault bit             */
#define ATA_STATUS_REG_BSY_BIT    0x80u /* Busy bit                    */

/* 28 bit LBA commands       */
#define ATA_READ28_CMD            0x20u /* LBA28 read sectors command  */
#define ATA_WRITE28_CMD           0x30u /* LBA28 write sectors command */

/* 48 bit LBA commands       */
#define ATA_READ48_CMD            0x24u /* LBA48 read sectors command  */
#define ATA_WRITE48_CMD           0x34u /* LBA48 write sectors command */

/* Shared commands           */
#define ATA_IDENTIFY_CMD          0xECu /* Identify drive command      */
#define ATA_FLUSH_CMD             0xE7u /* Flush cache command         */

static inline u16 ata_reg_port(ata_channel_s* channel, u8 reg) {
    return (u16)(channel->io + reg);
}

static inline u8 ata_read_reg(ata_channel_s* channel, u8 reg) {
    return inb(ata_reg_port(channel, reg));
}

static inline void ata_write_reg(ata_channel_s* channel, u8 reg, u8 val) {
    outb(ata_reg_port(channel, reg), val);
}

static inline void ata_write_ctrl(ata_channel_s* channel, u8 val) {
    outb(channel->ctrl, val);
}