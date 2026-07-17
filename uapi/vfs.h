#pragma once

#include <ipc.h>
#include <uapi/types.h>

#define VFS_SERVER_OPS(X) \
    X(1,  open)           \
    X(2,  close)          \
    X(3,  read)           \
    X(4,  write)          \
    X(5,  lseek)          \
    X(6,  stat)           \
    X(7,  fstat)          \
    X(8,  mkdir)          \
    X(9,  rmdir)          \
    X(10, unlink)         \
    X(11, link)           \
    X(12, symlink)        \
    X(13, readlink)       \
    X(14, rename)         \
    X(15, readdir)        \
    X(16, truncate)       \
    X(17, mount)          \
    X(18, umount)         \
    X(19, statfs)         \
    X(20, sync)

typedef enum vfs_server_op_e {
#define VFS_SERVER_OP_ENUM(id, name) VFS_SERVER_OP_##name = (id),
    VFS_SERVER_OPS(VFS_SERVER_OP_ENUM)
#undef VFS_SERVER_OP_ENUM
} vfs_server_op_e;

#define VFS_PATH_MAX   56u /* Path bytes per message, incl. NUL       */
#define VFS_NAME_MAX   40u /* Entry name bytes per dirent, incl. NUL  */
#define VFS_PAIR_MAX   28u /* Path bytes per two-path half, incl. NUL */
#define VFS_MOUNT_MAX  24u /* Path bytes per mount path, incl. NUL    */
#define VFS_FSNAME_MAX 12u /* Filesystem name bytes, incl. NUL        */
#define VFS_READ_MAX   56u /* Data bytes per read reply               */
#define VFS_WRITE_MAX  56u /* Data bytes per write request            */

#define VFS_O_RDONLY    0x00000u /* Open for reading only             */
#define VFS_O_WRONLY    0x00001u /* Open for writing only             */
#define VFS_O_RDWR      0x00002u /* Open for reading and writing      */
#define VFS_O_ACCMODE   0x00003u /* Access mode mask                  */
#define VFS_O_CREAT     0x00040u /* Create the file if absent         */
#define VFS_O_EXCL      0x00080u /* Fail creation if present          */
#define VFS_O_TRUNC     0x00200u /* Truncate to zero length on open   */
#define VFS_O_APPEND    0x00400u /* Position writes at end of file    */
#define VFS_O_DIRECTORY 0x10000u /* Fail unless target is a directory */
#define VFS_O_NOFOLLOW  0x20000u /* Do not follow a final symlink     */

#define VFS_SEEK_SET 0u /* Seek from start of file    */
#define VFS_SEEK_CUR 1u /* Seek from current position */
#define VFS_SEEK_END 2u /* Seek from end of file      */

#define VFS_IFMT  0xF000u /* File type mask */
#define VFS_IFDIR 0x4000u /* Directory      */
#define VFS_IFREG 0x8000u /* Regular file   */
#define VFS_IFLNK 0xA000u /* Symbolic link  */

#define VFS_ISDIR(mode) (((mode) & VFS_IFMT) == VFS_IFDIR)
#define VFS_ISREG(mode) (((mode) & VFS_IFMT) == VFS_IFREG)
#define VFS_ISLNK(mode) (((mode) & VFS_IFMT) == VFS_IFLNK)

#define VFS_DT(mode) ((mode) >> 12) /* Dirent type from mode bits */
#define VFS_DT_DIR   4u             /* Directory dirent           */
#define VFS_DT_REG   8u             /* Regular file dirent        */
#define VFS_DT_LNK   10u            /* Symbolic link dirent       */

typedef struct vfs_path_req_s {
    u32  flags;              /* VFS_O_* / operation flags */
    u32  mode;               /* Creation mode bits        */
    char path[VFS_PATH_MAX]; /* NUL-terminated path       */
} vfs_path_req_s;

typedef struct vfs_file_req_s {
    i32 fd;     /* File descriptor     */
    u32 count;  /* Byte count for read */
    i64 offset; /* Seek offset         */
    u32 whence; /* One of VFS_SEEK_*   */
    u32 pad;    /* Reserved            */
} vfs_file_req_s;

typedef struct vfs_write_req_s {
    i32 fd;                  /* File descriptor */
    u32 count;               /* Byte count      */
    u8  data[VFS_WRITE_MAX]; /* Inline data     */
} vfs_write_req_s;

typedef struct vfs_pair_req_s {
    u32  flags;              /* Operation flags                */
    u32  pad;                /* Reserved                       */
    char from[VFS_PAIR_MAX]; /* Source path or symlink content */
    char to[VFS_PAIR_MAX];   /* Destination path               */
} vfs_pair_req_s;

typedef struct vfs_trunc_req_s {
    i64  length;             /* New file length     */
    char path[VFS_PATH_MAX]; /* NUL-terminated path */
} vfs_trunc_req_s;

typedef struct vfs_mount_req_s {
    u32  flags;                  /* Mount flags         */
    char source[VFS_MOUNT_MAX];  /* Backing device path */
    char target[VFS_MOUNT_MAX];  /* Mount point path    */
    char fstype[VFS_FSNAME_MAX]; /* Filesystem name     */
} vfs_mount_req_s;

typedef struct vfs_stat_s {
    u32 dev;       /* Superblock identifier */
    u32 ino;       /* Inode number          */
    u32 mode;      /* Type and mode bits    */
    u32 nlink;     /* Hard link count       */
    i64 size;      /* File size in bytes    */
    u32 blocks;    /* Allocated block count */
    u32 blocksize; /* Block size in bytes   */
} vfs_stat_s;

typedef struct vfs_dirent_s {
    u32  ino;                /* Inode number        */
    u32  type;               /* One of VFS_DT_*     */
    char name[VFS_NAME_MAX]; /* NUL-terminated name */
} vfs_dirent_s;

typedef struct vfs_stat_reply_s {
    i32        ret;  /* Operation status */
    u32        pad;  /* Reserved         */
    vfs_stat_s stat; /* File attributes  */
} vfs_stat_reply_s;

typedef struct vfs_dirent_reply_s {
    i32          ret;    /* 1 = entry, 0 = end, < 0 = error */
    u32          pad;    /* Reserved                        */
    vfs_dirent_s dirent; /* Directory entry                 */
} vfs_dirent_reply_s;

typedef struct vfs_data_reply_s {
    i32 ret;                /* Byte count or negative error */
    u8  data[VFS_READ_MAX]; /* Inline data                  */
} vfs_data_reply_s;

typedef struct vfs_seek_reply_s {
    i32 ret;    /* Operation status  */
    u32 pad;    /* Reserved          */
    i64 offset; /* Resulting offset  */
} vfs_seek_reply_s;

typedef struct vfs_statfs_s {
    u32 blocks;    /* Total data blocks   */
    u32 bfree;     /* Free data blocks    */
    u32 files;     /* Total inodes        */
    u32 ffree;     /* Free inodes         */
    u32 blocksize; /* Block size in bytes */
} vfs_statfs_s;

typedef struct vfs_statfs_reply_s {
    i32          ret;    /* Operation status      */
    vfs_statfs_s statfs; /* Filesystem statistics */
} vfs_statfs_reply_s;

_Static_assert(sizeof(vfs_path_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_path_req_s size");
_Static_assert(sizeof(vfs_file_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_file_req_s size");
_Static_assert(sizeof(vfs_write_req_s)    <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_write_req_s size");
_Static_assert(sizeof(vfs_pair_req_s)     <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_pair_req_s size");
_Static_assert(sizeof(vfs_trunc_req_s)    <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_trunc_req_s size");
_Static_assert(sizeof(vfs_mount_req_s)    <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_mount_req_s size");
_Static_assert(sizeof(vfs_stat_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_stat_reply_s size");
_Static_assert(sizeof(vfs_dirent_reply_s) <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_dirent_reply_s size");
_Static_assert(sizeof(vfs_data_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_data_reply_s size");
_Static_assert(sizeof(vfs_seek_reply_s)   <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_seek_reply_s size");
_Static_assert(sizeof(vfs_statfs_reply_s) <= IPC_PAYLOAD_SZ, "Error: Invalid vfs_statfs_reply_s size");
