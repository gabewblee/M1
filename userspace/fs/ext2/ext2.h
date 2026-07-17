#pragma once

#include <userspace/fs/ext2/bcache.h>
#include <userspace/fs/ext2/ext2fs.h>
#include <userspace/fs/lib/fs.h>

typedef struct ext2_fs_s {
    fs_super_s*   sb;         /* Owning generic superblock    */
    ext2_dsuper_s dsb;        /* Cached on-disk superblock    */
    u32           block_size; /* Block size in bytes          */
    u32           block_log;  /* log2(block size)             */
    u32           groups;     /* Block group count            */
    u32           inode_size; /* On-disk inode size in bytes  */
    u32           ptrs;       /* Block pointers per block     */
    u32           gd_block;   /* First group descriptor block */
} ext2_fs_s;

typedef struct ext2_inode_s {
    fs_inode_s vfs;                 /* Embedded generic inode          */
    u32        data[EXT2_N_BLOCKS]; /* Block map or fast symlink bytes */
} ext2_inode_s;

extern const fs_driver_s ext2_driver; /* ext2 filesystem driver */

/**
 * ext2_fs_of - Returns the ext2 state behind the generic superblock @sb.
 * @sb: The generic superblock.
 * Returns: The ext2 filesystem state.
 */
static inline ext2_fs_s* ext2_fs_of(fs_super_s* sb) {
    return sb->private;
}

/**
 * ext2_inode_of - Returns the ext2 inode embedding the generic inode @inode.
 * @inode: The generic inode.
 * Returns: The embedding ext2 inode.
 */
static inline ext2_inode_s* ext2_inode_of(fs_inode_s* inode) {
    return get_container_of(inode, ext2_inode_s, vfs);
}

/**
 * ext2_sb_sync - Copies the cached superblock into the buffer cache.
 * @fs: The ext2 filesystem state.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_sb_sync(ext2_fs_s* fs);

/**
 * ext2_group_get - Returns a referenced buffer holding @group's descriptor.
 * @fs: The ext2 filesystem state.
 * @group: The block group index.
 * @desc: Filled with a pointer to the descriptor inside the buffer.
 * Returns: The referenced buffer, or NULL on failure.
 */
buf_s* ext2_group_get(ext2_fs_s* fs, u32 group, ext2_dgroup_s** desc);

/**
 * ext2_balloc - Allocates one data block.
 * @fs: The ext2 filesystem state.
 * @result: Filled with the new block number on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_balloc(ext2_fs_s* fs, u32* result);

/**
 * ext2_bfree - Frees the data block @block.
 * @fs: The ext2 filesystem state.
 * @block: The block number to free.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_bfree(ext2_fs_s* fs, u32 block);

/**
 * ext2_inode_alloc - Allocates one inode number.
 * @fs: The ext2 filesystem state.
 * @directory: Whether the inode will be a directory.
 * @result: Filled with the new inode number on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_inode_alloc(ext2_fs_s* fs, int directory, u32* result);

/**
 * ext2_inode_free - Frees the inode number @ino.
 * @fs: The ext2 filesystem state.
 * @ino: The inode number to free.
 * @directory: Whether the inode was a directory.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_inode_free(ext2_fs_s* fs, u32 ino, int directory);

/**
 * ext2_iget - Returns a referenced in-core inode for @ino, reading it from
 *             disk when absent.
 * @sb: The generic superblock.
 * @ino: The inode number.
 * @result: Filled with the referenced inode on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_iget(fs_super_s* sb, u32 ino, fs_inode_s** result);

/**
 * ext2_iwrite - Writes the in-core inode @inode back to the inode table.
 * @inode: The inode to write back.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_iwrite(fs_inode_s* inode);

/**
 * ext2_bmap - Maps logical block @index of @inode to a disk block.
 * @inode: The inode to map.
 * @index: The logical block index.
 * @allocate: Whether to allocate missing blocks.
 * @result: Filled with the disk block, 0 when unmapped and not allocating.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_bmap(fs_inode_s* inode, u32 index, int allocate, u32* result);

/**
 * ext2_itruncate - Resizes @inode to @length bytes, freeing surplus blocks.
 * @inode: The inode to resize.
 * @length: The new size in bytes.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_itruncate(fs_inode_s* inode, i64 length);

/**
 * ext2_ifree_blocks - Frees every data block of @inode.
 * @inode: The inode whose blocks are freed.
 */
void ext2_ifree_blocks(fs_inode_s* inode);

/**
 * ext2_file_read - Reads up to @count bytes of @inode at @offset into @buffer.
 * @inode: The inode to read from.
 * @buffer: The destination buffer.
 * @count: The maximum number of bytes to read.
 * @offset: The file offset to read at.
 * Returns: The number of bytes read, or a negative error code on failure.
 */
i32 ext2_file_read(fs_inode_s* inode, void* buffer, u32 count, i64 offset);

/**
 * ext2_file_write - Writes @count bytes from @buffer to @inode at @offset.
 * @inode: The inode to write to.
 * @buffer: The source buffer.
 * @count: The number of bytes to write.
 * @offset: The file offset to write at.
 * Returns: The number of bytes written, or a negative error code on failure.
 */
i32 ext2_file_write(fs_inode_s* inode, const void* buffer, u32 count, i64 offset);

/**
 * ext2_dir_find - Resolves @name in the directory @dir.
 * @dir: The directory inode.
 * @name: The NUL-terminated component name.
 * @result: Filled with the entry's inode number on success.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_dir_find(fs_inode_s* dir, const char* name, u32* result);

/**
 * ext2_dir_add - Adds the entry @name for @ino of type @file_type to @dir.
 * @dir: The directory inode.
 * @name: The NUL-terminated component name.
 * @ino: The entry's inode number.
 * @file_type: One of EXT2_FT_*.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_dir_add(fs_inode_s* dir, const char* name, u32 ino, u8 file_type);

/**
 * ext2_dir_remove - Removes the entry @name from @dir.
 * @dir: The directory inode.
 * @name: The NUL-terminated component name.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_dir_remove(fs_inode_s* dir, const char* name);

/**
 * ext2_dir_is_empty - Checks whether @dir holds only "." and "..".
 * @dir: The directory inode.
 * Returns: 1 if @dir is empty, 0 if not, or a negative error code on failure.
 */
i32 ext2_dir_is_empty(fs_inode_s* dir);

/**
 * ext2_dir_set_parent - Repoints the ".." entry of @dir at @parent.
 * @dir: The directory inode.
 * @parent: The new parent inode number.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 ext2_dir_set_parent(fs_inode_s* dir, u32 parent);

/**
 * ext2_readdir - Emits the directory entry of @dir at byte cookie @cookie.
 * @dir: The directory inode.
 * @cookie: The byte offset to resume at, 0 at the start.
 * @reply: Filled with the entry and the next cookie on success.
 * Returns: 1 when an entry was emitted, 0 at the end of the directory, or a
 *          negative error code on failure.
 */
i32 ext2_readdir(fs_inode_s* dir, u32 cookie, fs_readdir_reply_s* reply);
