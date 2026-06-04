#include "driver.h"
#include "regs.h"
#include "uapi/errno.h"
#include "uapi/io.h"

static ata_channel_s channels[2] = {
    { 
        .io     = 0x1F0,
        .ctrl   = 0x3F6,
        .irq    = 14,
        .select = 0xFF
    },
    { 
        .io     = 0x170,
        .ctrl   = 0x376,
        .irq    = 15,
        .select = 0xFF
    }
};

static ata_drv_s drvs[4];
static u32       present;

static i32 ata_probe_drv(ata_drv_s* drv) {
    ata_channel_s* channel = drv->channel;
    if (ata_read_status(channel) == 0xFF)
        return E_OK;

    ata_select_drv(channel, drv->slave);
    ata_write_reg(channel, ATA_SECTOR_CNT_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_LBALO_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_LBAMID_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_LBAHI_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_CMD_REG_OFFSET, ATA_IDENTIFY_CMD);

    u8 status = ata_read_status(channel);
    if (!status)
        return E_OK;

    while (status & ATA_STATUS_REG_BSY_BIT)
        status = ata_read_status(channel);

    u8 mid = ata_read_reg(channel, ATA_LBAMID_REG_OFFSET);
    u8 hi = ata_read_reg(channel, ATA_LBAHI_REG_OFFSET);
    if (mid || hi) {
        if (mid == 0x14 && hi == 0xEB)
            drv->kind = ATA_KIND_PATAPI;
        else if (mid == 0x69 && hi == 0x96)
            drv->kind = ATA_KIND_SATAPI;
        else if (mid == 0x3C && hi == 0xC3)
            drv->kind = ATA_KIND_SATA;
        return E_OK;
    }

    while (!(status & ATA_STATUS_REG_DRQ_BIT) && !(status & ATA_STATUS_REG_ERR_BIT))
        status = ata_read_status(channel);

    if (status & ATA_STATUS_REG_ERR_BIT)
        return E_OK;

    u16 words[256];
    for (u32 idx = 0; idx < 256; idx++)
        words[idx] = inw(ata_reg_port(channel, ATA_DATA_REG_OFFSET));

    drv->kind    = ATA_KIND_PATA;
    drv->present = 1;
    drv->lba48   = ((words[83] & (1u << 10)));
    drv->total28 = ((u32)words[61] << 16) | words[60];
    drv->total48 = ((u64)words[103] << 48) | ((u64)words[102] << 32) | ((u64)words[101] << 16) | words[100];
    present++;
    return E_OK;
}

u8 ata_read_status(const ata_channel_s* channel) {
    return ata_read_reg(channel, ATA_STATUS_REG_OFFSET);
}

u8 ata_read_alt_status(const ata_channel_s* channel) {
    return inb(channel->ctrl);
}

i32 ata_ready_wait(const ata_channel_s* channel) {
    for (u32 spins = 0; spins < 1000000; spins++) {
        u8 status = ata_read_status(channel);
        if (status & ATA_STATUS_REG_BSY_BIT)
            continue;

        if (status & (ATA_STATUS_REG_ERR_BIT | ATA_STATUS_REG_DF_BIT))
            return -(i32)E_FAULT;

        if (status & ATA_STATUS_REG_DRQ_BIT)
            return E_OK;
    }

    return -(i32)E_AGAIN;
}

void ata_select_wait(const ata_channel_s* channel) {
    for (u32 idx = 0; idx < 15; idx++)
        ata_read_alt_status(channel);
}

void ata_select_drv(ata_channel_s* channel, u8 slave) {
    u8 select = (u8)(0xA0 | ((slave & 1) << 4));
    if (channel->select == select)
        return;

    ata_write_reg(channel, ATA_DRIVE_REG_OFFSET, select);
    channel->select = select;
    ata_select_wait(channel);
}

ata_drv_s* ata_drv_lookup(u32 idx) {
    return (idx < 4) ? &drvs[idx] : 0;
}

u32 ata_get_present_drv_cnt(void) {
    return present;
}

void ata_drvs_init(void) {
    present = 0;
    for (u32 idx = 0; idx < 4; idx++) {
        drvs[idx]         = (ata_drv_s){0};
        drvs[idx].channel = &channels[idx >> 1];
        drvs[idx].slave   = (u8)(idx & 1);
        ata_write_ctrl(drvs[idx].channel, 0x00);
        ata_probe_drv(&drvs[idx]);
    }
}