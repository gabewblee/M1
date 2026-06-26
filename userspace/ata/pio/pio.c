#include <uapi/errno.h>
#include <uapi/io.h>
#include <userspace/ata/pio/pio.h>
#include <userspace/ata/regs.h>
#include <userspace/libc/syscall.h>

static i32 ata_data_wait(ata_channel_s* channel) {
    for (u32 spins = 0; spins < 1000000u; spins++) {
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

static i32 ata_done_wait(ata_channel_s* channel) {
    for (u32 spins = 0; spins < 1000000u; spins++) {
        u8 status = ata_read_status(channel);
        if (status & ATA_STATUS_REG_BSY_BIT)
            continue;

        if (status & (ATA_STATUS_REG_ERR_BIT | ATA_STATUS_REG_DF_BIT))
            return -(i32)E_FAULT;

        if (!(status & ATA_STATUS_REG_DRQ_BIT))
            return E_OK;
    }

    return -(i32)E_AGAIN;
}

static i32 ata_issue_lba28(ata_drv_s* drv, u64 lba, u8 cnt, u8 cmd) {
    ata_channel_s* channel = drv->channel;
    outb(ata_reg_port(channel, ATA_DRIVE_REG_OFFSET), (u8)(0xE0u | ((drv->slave & 1u) << 4) | ((lba >> 24) & 0x0Fu)));

    ata_select_wait(channel);
    ata_write_reg(channel, ATA_FEATURES_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_SECTOR_CNT_REG_OFFSET, cnt);
    ata_write_reg(channel, ATA_LBALO_REG_OFFSET, (u8)(lba & 0xFFu));
    ata_write_reg(channel, ATA_LBAMID_REG_OFFSET, (u8)((lba >> 8) & 0xFFu));
    ata_write_reg(channel, ATA_LBAHI_REG_OFFSET, (u8)((lba >> 16) & 0xFFu));
    ata_write_reg(channel, ATA_CMD_REG_OFFSET, cmd);
    return E_OK;
}

static i32 ata_issue_lba48(ata_drv_s* drv, u64 lba, u16 cnt, u8 cmd) {
    ata_channel_s* channel = drv->channel;
    outb(ata_reg_port(channel, ATA_DRIVE_REG_OFFSET), (u8)(0xE0u | ((drv->slave & 1u) << 4)));

    ata_select_wait(channel);
    ata_write_reg(channel, ATA_FEATURES_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_SECTOR_CNT_REG_OFFSET, (u8)((cnt >> 8) & 0xFFu));
    ata_write_reg(channel, ATA_LBALO_REG_OFFSET, (u8)((lba >> 24) & 0xFFu));
    ata_write_reg(channel, ATA_LBAMID_REG_OFFSET, (u8)((lba >> 32) & 0xFFu));
    ata_write_reg(channel, ATA_LBAHI_REG_OFFSET, (u8)((lba >> 40) & 0xFFu));

    ata_write_reg(channel, ATA_FEATURES_REG_OFFSET, 0);
    ata_write_reg(channel, ATA_SECTOR_CNT_REG_OFFSET, (u8)(cnt & 0xFFu));
    ata_write_reg(channel, ATA_LBALO_REG_OFFSET, (u8)(lba & 0xFFu));
    ata_write_reg(channel, ATA_LBAMID_REG_OFFSET, (u8)((lba >> 8) & 0xFFu));
    ata_write_reg(channel, ATA_LBAHI_REG_OFFSET, (u8)((lba >> 16) & 0xFFu));
    ata_write_reg(channel, ATA_CMD_REG_OFFSET, cmd);
    return E_OK;
}

static i32 ata_read_lba28(ata_drv_s* drv, u64 lba, u8 cnt, void* buf) {
    ata_channel_s* channel = drv->channel;
    u16* words = (u16*)buf;

    ata_issue_lba28(drv, lba, cnt, ATA_READ28_CMD);
    u32 total = (!cnt) ? 256u : (u32)cnt;

    for (u32 sec = 0; sec < total; sec++) {
        i32 ret = sys_irq_wait_for(channel->irq);
        if (ret != E_OK)
            return ret;

        ret = ata_data_wait(channel);
        if (ret != E_OK)
            return ret;

        for (u32 idx = 0; idx < 256u; idx++)
            words[sec * 256u + idx] = inw(ata_reg_port(channel, ATA_DATA_REG_OFFSET));
    }

    return E_OK;
}

static i32 ata_write_lba28(ata_drv_s* drv, u64 lba, u8 cnt, void* buf) {
    ata_channel_s* channel = drv->channel;
    u16* words = (u16*)buf;

    ata_issue_lba28(drv, lba, cnt, ATA_WRITE28_CMD);
    u32 total = (!cnt) ? 256u : (u32)cnt;

    for (u32 sec = 0; sec < total; sec++) {
        i32 ret = sys_irq_wait_for(channel->irq);
        if (ret != E_OK)
            return ret;

        ret = ata_data_wait(channel);
        if (ret != E_OK)
            return ret;

        for (u32 idx = 0; idx < 256u; idx++)
            outw(ata_reg_port(channel, ATA_DATA_REG_OFFSET), words[sec * 256u + idx]);
    }

    ata_write_reg(channel, ATA_CMD_REG_OFFSET, ATA_FLUSH_CMD);
    if (sys_irq_wait_for(channel->irq) != E_OK)
        return -(i32)E_AGAIN;

    return ata_done_wait(channel);
}

static i32 ata_read_lba48(ata_drv_s* drv, u64 lba, u16 cnt, void* buf) {
    ata_channel_s* channel = drv->channel;
    u16* words = (u16*)buf;

    ata_issue_lba48(drv, lba, cnt, ATA_READ48_CMD);
    u32 total = (!cnt) ? 65536u : (u32)cnt;

    for (u32 sec = 0; sec < total; sec++) {
        i32 ret = sys_irq_wait_for(channel->irq);
        if (ret != E_OK)
            return ret;

        ret = ata_data_wait(channel);
        if (ret != E_OK)
            return ret;

        for (u32 idx = 0; idx < 256u; idx++)
            words[sec * 256u + idx] = inw(ata_reg_port(channel, ATA_DATA_REG_OFFSET));
    }

    return E_OK;
}

static i32 ata_write_lba48(ata_drv_s* drv, u64 lba, u16 cnt, void* buf) {
    ata_channel_s* channel = drv->channel;
    u16* words = (u16*)buf;

    ata_issue_lba48(drv, lba, cnt, ATA_WRITE48_CMD);
    u32 total = (!cnt) ? 65536u : (u32)cnt;
    for (u32 sec = 0; sec < total; sec++) {
        i32 ret = sys_irq_wait_for(channel->irq);
        if (ret != E_OK)
            return ret;

        ret = ata_data_wait(channel);
        if (ret != E_OK)
            return ret;

        for (u32 idx = 0; idx < 256u; idx++)
            outw(ata_reg_port(channel, ATA_DATA_REG_OFFSET), words[sec * 256u + idx]);
    }

    ata_write_reg(channel, ATA_CMD_REG_OFFSET, ATA_FLUSH_CMD);
    if (sys_irq_wait_for(channel->irq) != E_OK)
        return -(i32)E_AGAIN;

    return ata_done_wait(channel);
}

i32 ata_pio_read(ata_drv_s* drv, u64 lba, u16 cnt, void* buf) {
    if (!drv || !drv->present || !buf)
        return -(i32)E_INVAL;

    if (drv->lba48)
        return ata_read_lba48(drv, lba, cnt, buf);

    if (lba > 0x0FFFFFFFu)
        return -(i32)E_INVAL;

    if (cnt > 0xFFu)
        return -(i32)E_INVAL;

    return ata_read_lba28(drv, lba, (u8)cnt, buf);
}

i32 ata_pio_write(ata_drv_s* drv, u64 lba, u16 cnt, void* buf) {
    if (!drv || !drv->present || !buf)
        return -(i32)E_INVAL;

    if (drv->lba48)
        return ata_write_lba48(drv, lba, cnt, buf);

    if (lba > 0x0FFFFFFFu)
        return -(i32)E_INVAL;

    if (cnt > 0xFFu)
        return -(i32)E_INVAL;

    return ata_write_lba28(drv, lba, (u8)cnt, buf);
}