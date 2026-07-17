#include <userspace/fs/ext2/ext2.h>
#include <userspace/fs/lib/icache.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/string.h>

static buf_s* inode_buf(ext2_fs_s* fs, u32 ino, ext2_dinode_s** raw) {
    u32 index = ino - 1u;
    u32 group = index / fs->dsb.s_inodes_per_group;
    u32 slot  = index % fs->dsb.s_inodes_per_group;
    if (ino == 0 || group >= fs->groups)
        return NULL;

    ext2_dgroup_s* desc;
    buf_s* gd = ext2_group_get(fs, group, &desc);
    if (!gd)
        return NULL;

    u32 per_block = fs->block_size / fs->inode_size;
    u32 block     = desc->bg_inode_table + slot / per_block;
    u32 offset    = (slot % per_block) * fs->inode_size;
    brelse(gd);

    buf_s* buffer = bread(block);
    if (!buffer)
        return NULL;

    *raw = (ext2_dinode_s*)(buffer->data + offset);
    return buffer;
}

i32 ext2_iget(fs_super_s* sb, u32 ino, fs_inode_s** result) {
    fs_inode_s* inode = fs_iget(sb, ino);
    if (inode) {
        *result = inode;
        return E_OK;
    }

    ext2_fs_s* fs = ext2_fs_of(sb);
    ext2_dinode_s* raw;
    buf_s* buffer = inode_buf(fs, ino, &raw);
    if (!buffer)
        return -(i32)E_IO;

    inode = fs_inode_new(sb);
    if (!inode) {
        brelse(buffer);
        return -(i32)E_NOMEM;
    }

    inode->ino    = ino;
    inode->mode   = raw->i_mode;
    inode->nlink  = raw->i_links_count;
    inode->size   = raw->i_size;
    inode->blocks = raw->i_blocks / (fs->block_size / 512u);
    memcpy(ext2_inode_of(inode)->data, raw->i_block, sizeof(raw->i_block));
    brelse(buffer);

    fs_inode_hash(inode);
    *result = inode;
    return E_OK;
}

i32 ext2_iwrite(fs_inode_s* inode) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    ext2_dinode_s* raw;
    buf_s* buffer = inode_buf(fs, inode->ino, &raw);
    if (!buffer)
        return -(i32)E_IO;

    raw->i_mode        = (u16)inode->mode;
    raw->i_links_count = (u16)inode->nlink;
    raw->i_size        = (u32)inode->size;
    raw->i_blocks      = inode->blocks * (fs->block_size / 512u);
    raw->i_dtime       = inode->nlink ? 0u : EXT2_DTIME_DEAD;
    memcpy(raw->i_block, ext2_inode_of(inode)->data, sizeof(raw->i_block));
    bdirty(buffer);
    brelse(buffer);
    return E_OK;
}

static i32 indirect_walk(fs_inode_s* inode, u32 block, u32 slot, int allocate, u32* result) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    buf_s* buffer = bread(block);
    if (!buffer)
        return -(i32)E_IO;

    u32* table = (u32*)buffer->data;
    u32 next = table[slot];
    if (!next && allocate) {
        i32 ret = ext2_balloc(fs, &next);
        if (ret != E_OK) {
            brelse(buffer);
            return ret;
        }

        buf_s* fresh = bread(next);
        if (!fresh) {
            ext2_bfree(fs, next);
            brelse(buffer);
            return -(i32)E_IO;
        }

        memset(fresh->data, 0, fs->block_size);
        bdirty(fresh);
        brelse(fresh);

        table[slot] = next;
        inode->blocks++;
        bdirty(buffer);
        ext2_iwrite(inode);
    }

    brelse(buffer);
    *result = next;
    return E_OK;
}

static i32 direct_map(fs_inode_s* inode, u32 slot, int allocate, u32* result) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    ext2_inode_s* node = ext2_inode_of(inode);
    u32 block = node->data[slot];
    if (!block && allocate) {
        i32 ret = ext2_balloc(fs, &block);
        if (ret != E_OK)
            return ret;

        buf_s* fresh = bread(block);
        if (!fresh) {
            ext2_bfree(fs, block);
            return -(i32)E_IO;
        }

        memset(fresh->data, 0, fs->block_size);
        bdirty(fresh);
        brelse(fresh);

        node->data[slot] = block;
        inode->blocks++;
        ext2_iwrite(inode);
    }

    *result = block;
    return E_OK;
}

i32 ext2_bmap(fs_inode_s* inode, u32 index, int allocate, u32* result) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    u32 ptrs = fs->ptrs;
    *result = 0;

    if (index < EXT2_NDIR_BLOCKS)
        return direct_map(inode, index, allocate, result);

    index -= EXT2_NDIR_BLOCKS;
    if (index < ptrs) {
        u32 table;
        i32 ret = direct_map(inode, EXT2_IND_BLOCK, allocate, &table);
        if (ret != E_OK || !table)
            return ret;

        return indirect_walk(inode, table, index, allocate, result);
    }

    index -= ptrs;
    if (index < ptrs * ptrs) {
        u32 outer;
        i32 ret = direct_map(inode, EXT2_DIND_BLOCK, allocate, &outer);
        if (ret != E_OK || !outer)
            return ret;

        u32 inner;
        ret = indirect_walk(inode, outer, index / ptrs, allocate, &inner);
        if (ret != E_OK || !inner)
            return ret;

        return indirect_walk(inode, inner, index % ptrs, allocate, result);
    }

    return -(i32)E_NOSPC;
}

static void free_indirect(ext2_fs_s* fs, fs_inode_s* inode, u32 block, u32 depth) {
    buf_s* buffer = bread(block);
    if (buffer) {
        u32* table = (u32*)buffer->data;
        for (u32 i = 0; i < fs->ptrs; i++) {
            if (!table[i])
                continue;

            if (depth)
                free_indirect(fs, inode, table[i], depth - 1u);

            ext2_bfree(fs, table[i]);
            inode->blocks--;
        }

        brelse(buffer);
    }

    ext2_bfree(fs, block);
    inode->blocks--;
}

void ext2_ifree_blocks(fs_inode_s* inode) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    ext2_inode_s* node = ext2_inode_of(inode);

    for (u32 i = 0; i < EXT2_NDIR_BLOCKS; i++) {
        if (!node->data[i])
            continue;

        ext2_bfree(fs, node->data[i]);
        node->data[i] = 0;
        inode->blocks--;
    }

    if (node->data[EXT2_IND_BLOCK]) {
        free_indirect(fs, inode, node->data[EXT2_IND_BLOCK], 0);
        node->data[EXT2_IND_BLOCK] = 0;
    }

    if (node->data[EXT2_DIND_BLOCK]) {
        free_indirect(fs, inode, node->data[EXT2_DIND_BLOCK], 1);
        node->data[EXT2_DIND_BLOCK] = 0;
    }

    inode->size = 0;
}

static i32 bmap_clear(fs_inode_s* inode, u32 index, u32* result) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    ext2_inode_s* node = ext2_inode_of(inode);
    u32 ptrs = fs->ptrs;
    *result = 0;

    if (index < EXT2_NDIR_BLOCKS) {
        *result = node->data[index];
        node->data[index] = 0;
        return E_OK;
    }

    index -= EXT2_NDIR_BLOCKS;
    u32 table = 0, slot = 0;
    if (index < ptrs) {
        table = node->data[EXT2_IND_BLOCK];
        slot  = index;
    } else {
        index -= ptrs;
        if (index >= ptrs * ptrs)
            return E_OK;

        u32 outer = node->data[EXT2_DIND_BLOCK];
        if (!outer)
            return E_OK;

        i32 ret = indirect_walk(inode, outer, index / ptrs, 0, &table);
        if (ret != E_OK)
            return ret;

        slot = index % ptrs;
    }

    if (!table)
        return E_OK;

    buf_s* buffer = bread(table);
    if (!buffer)
        return -(i32)E_IO;

    u32* entries = (u32*)buffer->data;
    *result = entries[slot];
    if (entries[slot]) {
        entries[slot] = 0;
        bdirty(buffer);
    }

    brelse(buffer);
    return E_OK;
}

i32 ext2_itruncate(fs_inode_s* inode, i64 length) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    if (length == 0) {
        ext2_ifree_blocks(inode);
        return ext2_iwrite(inode);
    }

    if (length < inode->size) {
        u32 keep = (u32)((length + fs->block_size - 1) >> fs->block_log);
        u32 last = (u32)((inode->size + fs->block_size - 1) >> fs->block_log);
        for (u32 index = keep; index < last; index++) {
            u32 block;
            if (bmap_clear(inode, index, &block) != E_OK || !block)
                continue;

            ext2_bfree(fs, block);
            inode->blocks--;
        }

        u32 offset = (u32)length & (fs->block_size - 1u);
        if (offset) {
            u32 block;
            if (ext2_bmap(inode, (u32)(length >> fs->block_log), 0, &block) == E_OK && block) {
                buf_s* buffer = bread(block);
                if (buffer) {
                    memset(buffer->data + offset, 0, fs->block_size - offset);
                    bdirty(buffer);
                    brelse(buffer);
                }
            }
        }
    }

    inode->size = length;
    return ext2_iwrite(inode);
}

i32 ext2_file_read(fs_inode_s* inode, void* buffer, u32 count, i64 offset) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    if (offset >= inode->size)
        return 0;

    if ((i64)count > inode->size - offset)
        count = (u32)(inode->size - offset);

    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(offset >> fs->block_log);
        u32 skip  = (u32)offset & (fs->block_size - 1u);
        u32 chunk = min(count - done, fs->block_size - skip);

        u32 block;
        i32 ret = ext2_bmap(inode, index, 0, &block);
        if (ret != E_OK)
            return done ? (i32)done : ret;

        if (block) {
            buf_s* cached = bread(block);
            if (!cached)
                return done ? (i32)done : -(i32)E_IO;

            memcpy((u8*)buffer + done, cached->data + skip, chunk);
            brelse(cached);
        } else {
            memset((u8*)buffer + done, 0, chunk);
        }

        done += chunk;
        offset += chunk;
    }

    return (i32)done;
}

i32 ext2_file_write(fs_inode_s* inode, const void* buffer, u32 count, i64 offset) {
    ext2_fs_s* fs = ext2_fs_of(inode->sb);
    u32 done = 0;
    while (done < count) {
        u32 index = (u32)(offset >> fs->block_log);
        u32 skip  = (u32)offset & (fs->block_size - 1u);
        u32 chunk = min(count - done, fs->block_size - skip);

        u32 block;
        i32 ret = ext2_bmap(inode, index, 1, &block);
        if (ret != E_OK)
            break;

        buf_s* cached = bread(block);
        if (!cached)
            break;

        memcpy(cached->data + skip, (const u8*)buffer + done, chunk);
        bdirty(cached);
        brelse(cached);

        done += chunk;
        offset += chunk;
    }

    if (!done)
        return -(i32)E_NOSPC;

    if (offset > inode->size)
        inode->size = offset;

    ext2_iwrite(inode);
    return (i32)done;
}
