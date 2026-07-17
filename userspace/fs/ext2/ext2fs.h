#pragma once

#include <uapi/types.h>

#define EXT2_MAGIC          0xEF53u /* On-disk superblock magic       */
#define EXT2_SB_OFF         1024u   /* Superblock byte offset         */
#define EXT2_SB_SZ          1024u   /* Superblock byte size           */
#define EXT2_MIN_BLOCK_LOG  10u     /* log2 of the minimum block size */
#define EXT2_MAX_BLOCK_LOG  12u     /* log2 of the maximum block size */
#define EXT2_ROOT_INO       2u      /* Root directory inode number    */
#define EXT2_FIRST_INO      11u     /* First non-reserved inode       */
#define EXT2_NAME_MAX       255u    /* Directory entry name limit     */
#define EXT2_NDIR_BLOCKS    12u     /* Direct block pointers          */
#define EXT2_IND_BLOCK      12u     /* Single indirect pointer index  */
#define EXT2_DIND_BLOCK     13u     /* Double indirect pointer index  */
#define EXT2_TIND_BLOCK     14u     /* Triple indirect pointer index  */
#define EXT2_N_BLOCKS       15u     /* Block pointers per inode       */
#define EXT2_FAST_LINK_MAX  60u     /* Inline symlink byte capacity   */
#define EXT2_GOOD_OLD_REV   0u      /* Revision 0, fixed inode size   */
#define EXT2_GOOD_OLD_ISIZE 128u    /* Revision 0 inode size          */
#define EXT2_VALID_FS       1u      /* Cleanly unmounted state        */
#define EXT2_DTIME_DEAD     0x60000000u /* Deletion stamp, no clock   */

#define EXT2_S_IFMT  0xF000u /* File type mask */
#define EXT2_S_IFLNK 0xA000u /* Symbolic link  */
#define EXT2_S_IFREG 0x8000u /* Regular file   */
#define EXT2_S_IFDIR 0x4000u /* Directory      */

#define EXT2_FT_UNKNOWN 0u /* Unknown dirent type  */
#define EXT2_FT_REG     1u /* Regular file dirent  */
#define EXT2_FT_DIR     2u /* Directory dirent     */
#define EXT2_FT_SYMLINK 7u /* Symbolic link dirent */

#define EXT2_DIRENT_BASE  8u                              /* Dirent header bytes    */
#define EXT2_DIRENT_SZ(n) ((EXT2_DIRENT_BASE + (n) + 3u) & ~3u) /* Aligned dirent size */

typedef struct ext2_dsuper_s {
    u32 s_inodes_count;      /* Total inodes                  */
    u32 s_blocks_count;      /* Total blocks                  */
    u32 s_r_blocks_count;    /* Reserved blocks               */
    u32 s_free_blocks_count; /* Free blocks                   */
    u32 s_free_inodes_count; /* Free inodes                   */
    u32 s_first_data_block;  /* First data block              */
    u32 s_log_block_size;    /* log2(block size) - 10         */
    u32 s_log_frag_size;     /* log2(fragment size) - 10      */
    u32 s_blocks_per_group;  /* Blocks per group              */
    u32 s_frags_per_group;   /* Fragments per group           */
    u32 s_inodes_per_group;  /* Inodes per group              */
    u32 s_mtime;             /* Last mount time               */
    u32 s_wtime;             /* Last write time               */
    u16 s_mnt_count;         /* Mounts since last check       */
    u16 s_max_mnt_count;     /* Mounts allowed between checks */
    u16 s_magic;             /* EXT2_MAGIC                    */
    u16 s_state;             /* Filesystem state              */
    u16 s_errors;            /* Error handling policy         */
    u16 s_minor_rev_level;   /* Minor revision level          */
    u32 s_lastcheck;         /* Last check time               */
    u32 s_checkinterval;     /* Maximum time between checks   */
    u32 s_creator_os;        /* Creator operating system      */
    u32 s_rev_level;         /* Revision level                */
    u16 s_def_resuid;        /* Default reserved-block uid    */
    u16 s_def_resgid;        /* Default reserved-block gid    */
    u32 s_first_ino;         /* First non-reserved inode      */
    u16 s_inode_size;        /* On-disk inode size            */
    u16 s_block_group_nr;    /* Group hosting this superblock */
    u32 s_feature_compat;    /* Compatible feature flags      */
    u32 s_feature_incompat;  /* Incompatible feature flags    */
    u32 s_feature_ro_compat; /* Read-only-compatible features */
    u8  s_uuid[16];          /* Volume UUID                   */
    char s_volume_name[16];  /* Volume label                  */
} ext2_dsuper_s;

typedef struct ext2_dgroup_s {
    u32 bg_block_bitmap;      /* Block bitmap block       */
    u32 bg_inode_bitmap;      /* Inode bitmap block       */
    u32 bg_inode_table;       /* First inode table block  */
    u16 bg_free_blocks_count; /* Free blocks in the group */
    u16 bg_free_inodes_count; /* Free inodes in the group */
    u16 bg_used_dirs_count;   /* Directories in the group */
    u16 bg_pad;               /* Reserved                 */
    u32 bg_reserved[3];       /* Reserved                 */
} ext2_dgroup_s;

typedef struct ext2_dinode_s {
    u16 i_mode;                  /* Type and mode bits        */
    u16 i_uid;                   /* Owner uid                 */
    u32 i_size;                  /* Size in bytes             */
    u32 i_atime;                 /* Access time               */
    u32 i_ctime;                 /* Change time               */
    u32 i_mtime;                 /* Modification time         */
    u32 i_dtime;                 /* Deletion time             */
    u16 i_gid;                   /* Owner gid                 */
    u16 i_links_count;           /* Hard link count           */
    u32 i_blocks;                /* Allocated 512-byte units  */
    u32 i_flags;                 /* Inode flags               */
    u32 i_osd1;                  /* OS-dependent field        */
    u32 i_block[EXT2_N_BLOCKS];  /* Block map or fast symlink */
    u32 i_generation;            /* File version for NFS      */
    u32 i_file_acl;              /* Extended attribute block  */
    u32 i_dir_acl;               /* High size bits            */
    u32 i_faddr;                 /* Fragment address          */
    u8  i_osd2[12];              /* OS-dependent fields       */
} ext2_dinode_s;

typedef struct ext2_dirent_s {
    u32 inode;     /* Entry inode, 0 when unused */
    u16 rec_len;   /* Record length in bytes     */
    u8  name_len;  /* Name length in bytes       */
    u8  file_type; /* One of EXT2_FT_*           */
    char name[];   /* Name bytes, unterminated   */
} ext2_dirent_s;

_Static_assert(sizeof(ext2_dgroup_s) == 32,  "Error: Invalid ext2_dgroup_s size");
_Static_assert(sizeof(ext2_dinode_s) == 128, "Error: Invalid ext2_dinode_s size");
