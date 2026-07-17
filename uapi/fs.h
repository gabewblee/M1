#pragma once

#include <ipc.h>
#include <uapi/types.h>
#include <uapi/vfs.h>

#define FS_SERVER_OPS(X) \
    X(1,  mount)         \
    X(2,  umount)        \
    X(3,  sync)          \
    X(4,  statfs)        \
    X(5,  lookup)        \
    X(6,  forget)        \
    X(7,  getattr)       \
    X(8,  create)        \
    X(9,  mkdir)         \
    X(10, rmdir)         \
    X(11, unlink)        \
    X(12, link)          \
    X(13, symlink)       \
    X(14, readlink)      \
    X(15, rename)        \
    X(16, truncate)      \
    X(17, read)          \
    X(18, write)         \
    X(19, readdir)

typedef enum fs_server_op_e {
#define FS_SERVER_OP_ENUM(id, name) FS_SERVER_OP_##name = (id),
    FS_SERVER_OPS(FS_SERVER_OP_ENUM)
#undef FS_SERVER_OP_ENUM
} fs_server_op_e;

#define FS_SOURCE_MAX 24u /* Mount source bytes, incl. NUL        */
#define FS_LINK_MAX   28u /* Symlink name/target bytes, incl. NUL */
#define FS_RENAME_MAX 24u /* Rename component bytes, incl. NUL    */
#define FS_READ_MAX   56u /* Data bytes per read reply            */
#define FS_WRITE_MAX  40u /* Data bytes per write request         */

#define FS_WRITE_APPEND 0x1u /* Write at end of file, ignore offset */

typedef struct fs_mount_req_s {
    u32  flags;                 /* Mount flags         */
    char source[FS_SOURCE_MAX]; /* Backing device name */
} fs_mount_req_s;

typedef struct fs_mount_reply_s {
    i32 ret;       /* Operation status        */
    u32 sb;        /* Superblock handle       */
    u32 root;      /* Root node id            */
    u32 blocksize; /* Block size in bytes     */
    u32 magic;     /* Filesystem magic number */
} fs_mount_reply_s;

typedef struct fs_sb_req_s {
    u32 sb; /* Superblock handle */
} fs_sb_req_s;

typedef struct fs_statfs_reply_s {
    i32 ret;       /* Operation status    */
    u32 blocks;    /* Total data blocks   */
    u32 bfree;     /* Free data blocks    */
    u32 files;     /* Total inodes        */
    u32 ffree;     /* Free inodes         */
    u32 blocksize; /* Block size in bytes */
} fs_statfs_reply_s;

typedef struct fs_name_req_s {
    u32  sb;                 /* Superblock handle     */
    u32  dir;                /* Parent directory node */
    u32  mode;               /* Creation mode bits    */
    char name[VFS_NAME_MAX]; /* NUL-terminated name   */
} fs_name_req_s;

typedef struct fs_node_req_s {
    u32 sb;    /* Superblock handle          */
    u32 ino;   /* Target node id             */
    u32 count; /* Reference count for forget */
} fs_node_req_s;

typedef struct fs_entry_reply_s {
    i32 ret;    /* Operation status      */
    u32 ino;    /* Node id               */
    u32 mode;   /* Type and mode bits    */
    u32 nlink;  /* Hard link count       */
    i64 size;   /* Size in bytes         */
    u32 blocks; /* Allocated block count */
} fs_entry_reply_s;

typedef struct fs_link_req_s {
    u32  sb;                 /* Superblock handle     */
    u32  ino;                /* Existing node id      */
    u32  dir;                /* Parent directory node */
    char name[VFS_NAME_MAX]; /* NUL-terminated name   */
} fs_link_req_s;

typedef struct fs_symlink_req_s {
    u32  sb;                  /* Superblock handle     */
    u32  dir;                 /* Parent directory node */
    char name[FS_LINK_MAX];   /* NUL-terminated name   */
    char target[FS_LINK_MAX]; /* NUL-terminated target */
} fs_symlink_req_s;

typedef struct fs_rename_req_s {
    u32  sb;                  /* Superblock handle          */
    u32  from_dir;            /* Source directory node      */
    u32  to_dir;              /* Destination directory node */
    char from[FS_RENAME_MAX]; /* Source component           */
    char to[FS_RENAME_MAX];   /* Destination component      */
} fs_rename_req_s;

typedef struct fs_trunc_req_s {
    u32 sb;     /* Superblock handle */
    u32 ino;    /* Target node id    */
    i64 length; /* New file length   */
} fs_trunc_req_s;

typedef struct fs_read_req_s {
    u32 sb;     /* Superblock handle */
    u32 ino;    /* Target node id    */
    u32 count;  /* Byte count        */
    u32 pad;    /* Reserved          */
    i64 offset; /* File offset       */
} fs_read_req_s;

typedef struct fs_write_req_s {
    u32 sb;                 /* Superblock handle */
    u32 ino;                /* Target node id    */
    u16 count;              /* Byte count        */
    u16 flags;              /* FS_WRITE_* flags  */
    i64 offset;             /* File offset       */
    u8  data[FS_WRITE_MAX]; /* Inline data       */
} fs_write_req_s;

typedef struct fs_write_reply_s {
    i32 ret; /* Byte count or negative error  */
    u32 pad; /* Reserved                      */
    i64 pos; /* File position after the write */
} fs_write_reply_s;

typedef struct fs_data_reply_s {
    i32 ret;               /* Byte count or negative error */
    u8  data[FS_READ_MAX]; /* Inline data                  */
} fs_data_reply_s;

typedef struct fs_readdir_req_s {
    u32 sb;     /* Superblock handle         */
    u32 ino;    /* Directory node id         */
    u32 cookie; /* Resume cookie, 0 at start */
} fs_readdir_req_s;

typedef struct fs_readdir_reply_s {
    i32  ret;                /* 1 = entry, 0 = end, < 0 = error */
    u32  cookie;             /* Cookie for the next entry       */
    u32  ino;                /* Entry node id                   */
    u32  type;               /* One of VFS_DT_*                 */
    char name[VFS_NAME_MAX]; /* NUL-terminated entry name       */
} fs_readdir_reply_s;

_Static_assert(sizeof(fs_mount_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid fs_mount_req_s size");
_Static_assert(sizeof(fs_mount_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid fs_mount_reply_s size");
_Static_assert(sizeof(fs_sb_req_s)        <= IPC_PAYLOAD_SZ, "Error: Invalid fs_sb_req_s size");
_Static_assert(sizeof(fs_statfs_reply_s)  <= IPC_PAYLOAD_SZ, "Error: Invalid fs_statfs_reply_s size");
_Static_assert(sizeof(fs_name_req_s)      <= IPC_PAYLOAD_SZ, "Error: Invalid fs_name_req_s size");
_Static_assert(sizeof(fs_node_req_s)      <= IPC_PAYLOAD_SZ, "Error: Invalid fs_node_req_s size");
_Static_assert(sizeof(fs_entry_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid fs_entry_reply_s size");
_Static_assert(sizeof(fs_link_req_s)      <= IPC_PAYLOAD_SZ, "Error: Invalid fs_link_req_s size");
_Static_assert(sizeof(fs_symlink_req_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid fs_symlink_req_s size");
_Static_assert(sizeof(fs_rename_req_s)    <= IPC_PAYLOAD_SZ, "Error: Invalid fs_rename_req_s size");
_Static_assert(sizeof(fs_trunc_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid fs_trunc_req_s size");
_Static_assert(sizeof(fs_read_req_s)      <= IPC_PAYLOAD_SZ, "Error: Invalid fs_read_req_s size");
_Static_assert(sizeof(fs_write_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid fs_write_req_s size");
_Static_assert(sizeof(fs_write_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid fs_write_reply_s size");
_Static_assert(sizeof(fs_data_reply_s)    <= IPC_PAYLOAD_SZ, "Error: Invalid fs_data_reply_s size");
_Static_assert(sizeof(fs_readdir_req_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid fs_readdir_req_s size");
_Static_assert(sizeof(fs_readdir_reply_s) <= IPC_PAYLOAD_SZ, "Error: Invalid fs_readdir_reply_s size");
