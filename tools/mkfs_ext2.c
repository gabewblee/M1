#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE       1024u
#define BLOCK_LOG        0u
#define BLOCKS_PER_GROUP (BLOCK_SIZE * 8u)
#define INODES_PER_GROUP 2048u
#define INODE_SIZE       128u
#define FIRST_DATA_BLOCK 1u
#define ROOT_INO         2u
#define FIRST_INO        11u
#define EXT2_MAGIC       0xEF53u
#define EXT2_VALID_FS    1u
#define EXT2_FT_DIR      2u
#define EXT2_DYNAMIC_REV 1u
#define FEATURE_FILETYPE 0x0002u

#define GD_PER_BLOCK     (BLOCK_SIZE / 32u)
#define ITB_BLOCKS       (INODES_PER_GROUP * INODE_SIZE / BLOCK_SIZE)
#define GROUP_OVERHEAD   (2u + 2u + ITB_BLOCKS) /* sb + gd copies, bitmaps, itable */

typedef struct superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
} superblock;

typedef struct groupdesc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} groupdesc;

typedef struct dinode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
} dinode;

typedef struct dirent {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[4];
} dirent;

static uint8_t* image;
static uint32_t total_blocks;
static uint32_t groups;

static uint8_t* block_at(uint32_t block) {
    return image + (size_t)block * BLOCK_SIZE;
}

static uint32_t group_base(uint32_t group) {
    return FIRST_DATA_BLOCK + group * BLOCKS_PER_GROUP;
}

static uint32_t blocks_in_group(uint32_t group) {
    uint32_t data = total_blocks - FIRST_DATA_BLOCK;
    uint32_t base = group * BLOCKS_PER_GROUP;
    uint32_t left = data - base;
    return left < BLOCKS_PER_GROUP ? left : BLOCKS_PER_GROUP;
}

static void bitmap_set(uint8_t* bitmap, uint32_t bit) {
    bitmap[bit >> 3] |= (uint8_t)(1u << (bit & 7u));
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <image> <size-mb>\n", argv[0]);
        return 1;
    }

    uint32_t mb = (uint32_t)strtoul(argv[2], NULL, 10);
    total_blocks = mb * (1024u * 1024u / BLOCK_SIZE);
    groups = (total_blocks - FIRST_DATA_BLOCK + BLOCKS_PER_GROUP - 1u) / BLOCKS_PER_GROUP;
    if (mb == 0 || groups * 32u > BLOCK_SIZE) {
        fprintf(stderr, "mkfs_ext2: unsupported image size\n");
        return 1;
    }

    size_t bytes = (size_t)total_blocks * BLOCK_SIZE;
    image = calloc(1, bytes);
    if (!image) {
        fprintf(stderr, "mkfs_ext2: out of memory\n");
        return 1;
    }

    groupdesc* gd = (groupdesc*)block_at(FIRST_DATA_BLOCK + 1u);
    uint32_t free_blocks = 0;
    for (uint32_t g = 0; g < groups; g++) {
        uint32_t base = group_base(g);
        gd[g].bg_block_bitmap = base + 2u;
        gd[g].bg_inode_bitmap = base + 3u;
        gd[g].bg_inode_table  = base + 4u;

        uint8_t* bbm = block_at(gd[g].bg_block_bitmap);
        uint32_t in_group = blocks_in_group(g);
        for (uint32_t b = 0; b < GROUP_OVERHEAD; b++)
            bitmap_set(bbm, b);

        for (uint32_t b = in_group; b < BLOCKS_PER_GROUP; b++)
            bitmap_set(bbm, b);

        gd[g].bg_free_blocks_count = (uint16_t)(in_group - GROUP_OVERHEAD);
        gd[g].bg_free_inodes_count = INODES_PER_GROUP;
        free_blocks += gd[g].bg_free_blocks_count;

        uint8_t* ibm = block_at(gd[g].bg_inode_bitmap);
        for (uint32_t i = INODES_PER_GROUP; i < BLOCK_SIZE * 8u; i++)
            bitmap_set(ibm, i);
    }

    uint8_t* ibm0 = block_at(gd[0].bg_inode_bitmap);
    for (uint32_t i = 0; i < FIRST_INO - 1u; i++)
        bitmap_set(ibm0, i);

    gd[0].bg_free_inodes_count = (uint16_t)(INODES_PER_GROUP - (FIRST_INO - 1u));
    gd[0].bg_used_dirs_count   = 1;

    uint32_t root_block = group_base(0) + GROUP_OVERHEAD;
    bitmap_set(block_at(gd[0].bg_block_bitmap), GROUP_OVERHEAD);
    gd[0].bg_free_blocks_count--;
    free_blocks--;

    dinode* root = (dinode*)block_at(gd[0].bg_inode_table) + (ROOT_INO - 1u);
    root->i_mode        = 040755;
    root->i_size        = BLOCK_SIZE;
    root->i_links_count = 2;
    root->i_blocks      = BLOCK_SIZE / 512u;
    root->i_block[0]    = root_block;

    dirent* dot = (dirent*)block_at(root_block);
    dot->inode     = ROOT_INO;
    dot->rec_len   = 12;
    dot->name_len  = 1;
    dot->file_type = EXT2_FT_DIR;
    memcpy(dot->name, ".", 1);

    dirent* dotdot = (dirent*)((uint8_t*)dot + 12);
    dotdot->inode     = ROOT_INO;
    dotdot->rec_len   = (uint16_t)(BLOCK_SIZE - 12u);
    dotdot->name_len  = 2;
    dotdot->file_type = EXT2_FT_DIR;
    memcpy(dotdot->name, "..", 2);

    superblock* sb = (superblock*)(image + 1024);
    sb->s_inodes_count      = groups * INODES_PER_GROUP;
    sb->s_blocks_count      = total_blocks;
    sb->s_free_blocks_count = free_blocks;
    sb->s_free_inodes_count = sb->s_inodes_count - (FIRST_INO - 1u);
    sb->s_first_data_block  = FIRST_DATA_BLOCK;
    sb->s_log_block_size    = BLOCK_LOG;
    sb->s_log_frag_size     = BLOCK_LOG;
    sb->s_blocks_per_group  = BLOCKS_PER_GROUP;
    sb->s_frags_per_group   = BLOCKS_PER_GROUP;
    sb->s_inodes_per_group  = INODES_PER_GROUP;
    sb->s_max_mnt_count     = 0xFFFF;
    sb->s_magic             = EXT2_MAGIC;
    sb->s_state             = EXT2_VALID_FS;
    sb->s_rev_level         = EXT2_DYNAMIC_REV;
    sb->s_first_ino         = FIRST_INO;
    sb->s_inode_size        = INODE_SIZE;
    sb->s_feature_incompat  = FEATURE_FILETYPE;
    memcpy(sb->s_uuid, "M1EXT2IMAGEUUID", 16);
    memcpy(sb->s_volume_name, "m1disk", 7);

    for (uint32_t g = 1; g < groups; g++) {
        memcpy(block_at(group_base(g)), image + 1024, BLOCK_SIZE);
        memcpy(block_at(group_base(g) + 1u), gd, BLOCK_SIZE);
    }

    FILE* out = fopen(argv[1], "wb");
    if (!out || fwrite(image, 1, bytes, out) != bytes || fclose(out)) {
        fprintf(stderr, "mkfs_ext2: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    printf("mkfs_ext2: %s: %u blocks, %u groups, %u inodes\n",
           argv[1], total_blocks, groups, sb->s_inodes_count);
    free(image);
    return 0;
}
