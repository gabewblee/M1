#pragma once

#include <uapi/uapi.h>
#include <userspace/libc/list.h>

#define FS_SUPER_MAX        4u   /* Superblocks per filesystem server */
#define FS_SUPER_KEEP_CACHE 0x1u /* Inodes stay cached when unpinned  */

typedef struct fs_super_s  fs_super_s;
typedef struct fs_inode_s  fs_inode_s;
typedef struct fs_driver_s fs_driver_s;

struct fs_super_s {
    u32                id;        /* Wire superblock handle   */
    u32                flags;     /* FS_SUPER_* flags         */
    u32                blocksize; /* Block size in bytes      */
    u32                magic;     /* Filesystem magic number  */
    u32                next_ino;  /* Node id allocator        */
    const fs_driver_s* driver;    /* Owning filesystem driver */
    fs_inode_s*        root;      /* Pinned root inode        */
    void*              private;   /* Filesystem-private data  */
    list_node_s        inodes;    /* In-core inodes           */
};

struct fs_inode_s {
    u32          ino;            /* Node id                 */
    u32          mode;           /* Type and mode bits      */
    u32          nlink;          /* Hard link count         */
    u32          count;          /* VFS reference count     */
    i64          size;           /* Size in bytes           */
    u32          blocks;         /* Allocated block count   */
    fs_super_s*  sb;             /* Owning superblock       */
    hlist_node_s hash_list_node; /* Inode hash chain link   */
    list_node_s  sb_list_node;   /* Link in superblock list */
};

struct fs_driver_s {
    const char* name;                                                                                   /* Filesystem name        */
    i32         (*init)(void);                                                                          /* Server initialization  */
    i32         (*mount)(fs_super_s* sb, const char* source, u32 flags);                                /* Fills a fresh @sb      */
    void        (*umount)(fs_super_s* sb);                                                              /* Releases @sb           */
    i32         (*sync)(fs_super_s* sb);                                                                /* Flushes dirty state    */
    void        (*statfs)(fs_super_s* sb, vfs_statfs_s* statfs);                                        /* Fills usage statistics */
    i32         (*lookup)(fs_inode_s* dir, const char* name, fs_inode_s** result);                      /* Resolves one component */
    i32         (*create)(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result);            /* Creates a regular file */
    i32         (*mkdir)(fs_inode_s* dir, const char* name, u32 mode, fs_inode_s** result);             /* Creates a directory    */
    i32         (*rmdir)(fs_inode_s* dir, const char* name);                                            /* Removes a directory    */
    i32         (*unlink)(fs_inode_s* dir, const char* name);                                           /* Removes a link         */
    i32         (*link)(fs_inode_s* inode, fs_inode_s* dir, const char* name);                          /* Adds a hard link       */
    i32         (*symlink)(fs_inode_s* dir, const char* name, const char* target, fs_inode_s** result); /* Creates a symlink      */
    i32         (*readlink)(fs_inode_s* inode, char* buffer, u32 size);                                 /* Reads symlink content  */
    i32         (*rename)(fs_inode_s* from_dir, const char* from, fs_inode_s* to_dir, const char* to);  /* Moves an entry         */
    i32         (*truncate)(fs_inode_s* inode, i64 length);                                             /* Resizes a file         */
    i32         (*read)(fs_inode_s* inode, void* buffer, u32 count, i64 offset);                        /* Reads file bytes       */
    i32         (*write)(fs_inode_s* inode, const void* buffer, u32 count, i64 offset);                 /* Writes file bytes      */
    i32         (*readdir)(fs_inode_s* dir, u32 cookie, fs_readdir_reply_s* reply);                     /* Emits one entry        */
    fs_inode_s* (*ialloc)(fs_super_s* sb);                                                              /* Allocates an inode     */
    void        (*ifree)(fs_inode_s* inode);                                                            /* Frees an inode         */
    void        (*ievict)(fs_inode_s* inode);                                                           /* Drops inode state      */
};
