#include <userspace/fs/ext2/ext2.h>
#include <userspace/libc/string.h>

static u32 blocks_in_group(const ext2_fs_s* fs, u32 group) {
    u32 data = fs->dsb.s_blocks_count - fs->dsb.s_first_data_block;
    u32 base = group * fs->dsb.s_blocks_per_group;
    u32 left = data - base;
    return (left < fs->dsb.s_blocks_per_group) ? left : fs->dsb.s_blocks_per_group;
}

i32 ext2_sb_sync(ext2_fs_s* fs) {
    u32 block  = EXT2_SB_OFF / fs->block_size;
    u32 offset = EXT2_SB_OFF % fs->block_size;
    buf_s* buffer = bread(block);
    if (!buffer)
        return -(i32)E_IO;

    memcpy(buffer->data + offset, &fs->dsb, sizeof(fs->dsb));
    bdirty(buffer);
    brelse(buffer);
    return E_OK;
}

buf_s* ext2_group_get(ext2_fs_s* fs, u32 group, ext2_dgroup_s** desc) {
    u32 per_block = fs->block_size / (u32)sizeof(ext2_dgroup_s);
    buf_s* buffer = bread(fs->gd_block + group / per_block);
    if (!buffer)
        return NULL;

    *desc = (ext2_dgroup_s*)buffer->data + group % per_block;
    return buffer;
}

static i32 bitmap_find_zero(const u8* bitmap, u32 limit) {
    for (u32 i = 0; i < limit; i++)
        if (!(bitmap[i >> 3] & (1u << (i & 7u))))
            return (i32)i;

    return -(i32)E_NOSPC;
}

static void bitmap_set(u8* bitmap, u32 bit) {
    bitmap[bit >> 3] |= (u8)(1u << (bit & 7u));
}

static void bitmap_clear(u8* bitmap, u32 bit) {
    bitmap[bit >> 3] &= (u8)~(1u << (bit & 7u));
}

i32 ext2_balloc(ext2_fs_s* fs, u32* result) {
    for (u32 group = 0; group < fs->groups; group++) {
        ext2_dgroup_s* desc;
        buf_s* gd = ext2_group_get(fs, group, &desc);
        if (!gd)
            return -(i32)E_IO;

        if (!desc->bg_free_blocks_count) {
            brelse(gd);
            continue;
        }

        buf_s* bitmap = bread(desc->bg_block_bitmap);
        if (!bitmap) {
            brelse(gd);
            return -(i32)E_IO;
        }

        i32 bit = bitmap_find_zero(bitmap->data, blocks_in_group(fs, group));
        if (bit < 0) {
            brelse(bitmap);
            brelse(gd);
            continue;
        }

        bitmap_set(bitmap->data, (u32)bit);
        desc->bg_free_blocks_count--;
        fs->dsb.s_free_blocks_count--;
        bdirty(bitmap);
        bdirty(gd);
        brelse(bitmap);
        brelse(gd);
        ext2_sb_sync(fs);

        *result = group * fs->dsb.s_blocks_per_group + fs->dsb.s_first_data_block + (u32)bit;
        return E_OK;
    }

    return -(i32)E_NOSPC;
}

i32 ext2_bfree(ext2_fs_s* fs, u32 block) {
    u32 index = block - fs->dsb.s_first_data_block;
    u32 group = index / fs->dsb.s_blocks_per_group;
    u32 bit   = index % fs->dsb.s_blocks_per_group;
    if (group >= fs->groups)
        return -(i32)E_INVAL;

    ext2_dgroup_s* desc;
    buf_s* gd = ext2_group_get(fs, group, &desc);
    if (!gd)
        return -(i32)E_IO;

    buf_s* bitmap = bread(desc->bg_block_bitmap);
    if (!bitmap) {
        brelse(gd);
        return -(i32)E_IO;
    }

    bitmap_clear(bitmap->data, bit);
    desc->bg_free_blocks_count++;
    fs->dsb.s_free_blocks_count++;
    bdirty(bitmap);
    bdirty(gd);
    brelse(bitmap);
    brelse(gd);
    return ext2_sb_sync(fs);
}

i32 ext2_inode_alloc(ext2_fs_s* fs, int directory, u32* result) {
    for (u32 group = 0; group < fs->groups; group++) {
        ext2_dgroup_s* desc;
        buf_s* gd = ext2_group_get(fs, group, &desc);
        if (!gd)
            return -(i32)E_IO;

        if (!desc->bg_free_inodes_count) {
            brelse(gd);
            continue;
        }

        buf_s* bitmap = bread(desc->bg_inode_bitmap);
        if (!bitmap) {
            brelse(gd);
            return -(i32)E_IO;
        }

        i32 bit = bitmap_find_zero(bitmap->data, fs->dsb.s_inodes_per_group);
        if (bit < 0) {
            brelse(bitmap);
            brelse(gd);
            continue;
        }

        bitmap_set(bitmap->data, (u32)bit);
        desc->bg_free_inodes_count--;
        fs->dsb.s_free_inodes_count--;
        if (directory)
            desc->bg_used_dirs_count++;

        bdirty(bitmap);
        bdirty(gd);
        brelse(bitmap);
        brelse(gd);
        ext2_sb_sync(fs);

        *result = group * fs->dsb.s_inodes_per_group + (u32)bit + 1u;
        return E_OK;
    }

    return -(i32)E_NOSPC;
}

i32 ext2_inode_free(ext2_fs_s* fs, u32 ino, int directory) {
    u32 index = ino - 1u;
    u32 group = index / fs->dsb.s_inodes_per_group;
    u32 bit   = index % fs->dsb.s_inodes_per_group;
    if (group >= fs->groups)
        return -(i32)E_INVAL;

    ext2_dgroup_s* desc;
    buf_s* gd = ext2_group_get(fs, group, &desc);
    if (!gd)
        return -(i32)E_IO;

    buf_s* bitmap = bread(desc->bg_inode_bitmap);
    if (!bitmap) {
        brelse(gd);
        return -(i32)E_IO;
    }

    bitmap_clear(bitmap->data, bit);
    desc->bg_free_inodes_count++;
    fs->dsb.s_free_inodes_count++;
    if (directory)
        desc->bg_used_dirs_count--;

    bdirty(bitmap);
    bdirty(gd);
    brelse(bitmap);
    brelse(gd);
    return ext2_sb_sync(fs);
}
