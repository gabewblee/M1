#include <userspace/libc/hash.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/file.h>
#include <userspace/vfs/mount.h>
#include <userspace/vfs/namei.h>
#include <userspace/vfs/selftest.h>

static u32 passed;
static u32 failed;
static u8  buffer[16384];
static u8  pattern[16384];

static void check(int condition, const char* name) {
    if (condition) {
        passed++;
    } else {
        failed++;
        dprintf("[VFS] FAIL %s\n", name);
    }
}

static void check_directories(void) {
    vfs_stat_s stat;
    check(do_mkdir("/usr", 0755) == E_OK,                   "mkdir");
    check(do_mkdir("/usr/share", 0755) == E_OK,             "mkdir nested");
    check(do_mkdir("/tmp", 0755) == E_OK,                   "mkdir sibling");
    check(do_mkdir("/usr/local/", 0755) == E_OK,            "mkdir trailing slash");
    check(do_mkdir("/usr", 0755) == -(i32)E_EXIST,          "mkdir duplicate fails");
    check(do_mkdir("/absent/child", 0755) == -(i32)E_NOENT, "mkdir under missing parent fails");
    check(do_stat("/usr", 0, &stat) == E_OK && VFS_ISDIR(stat.mode) && stat.nlink == 4, "directory link count");
    check(do_mkdir("/0123456789012345678901234567890123456789ABC", 0755) == -(i32)E_NAMELONG, "long component rejected");
}

static void check_files(void) {
    vfs_stat_s stat;
    i64 position = -1;

    i32 fd = fd_open(0, "/usr/hello", VFS_O_CREAT | VFS_O_RDWR, 0644);
    check(fd >= 0, "open O_CREAT");
    check(fd_write(0, fd, "hello, vfs", 10) == 10, "write");
    check(fd_seek(0, fd, 0, VFS_SEEK_SET, &position) == E_OK && position == 0, "lseek SET");
    check(fd_read(0, fd, buffer, 16) == 10 && memcmp(buffer, "hello, vfs", 10) == 0, "read back");
    check(fd_stat(0, fd, &stat) == E_OK && stat.size == 10 && VFS_ISREG(stat.mode), "fstat");
    check(fd_close(0, fd) == E_OK, "close");
    check(fd_close(0, fd) == -(i32)E_BADF, "double close fails");

    check(fd_open(0, "/usr/hello", VFS_O_CREAT | VFS_O_EXCL | VFS_O_RDWR, 0644) == -(i32)E_EXIST, "O_EXCL on existing fails");

    fd = fd_open(0, "/usr/hello", VFS_O_RDONLY, 0);
    check(fd >= 0 && fd_write(0, fd, "x", 1) == -(i32)E_BADF, "write on O_RDONLY fails");
    fd_close(0, fd);

    fd = fd_open(0, "/usr/hello", VFS_O_WRONLY | VFS_O_APPEND, 0);
    check(fd >= 0 && fd_write(0, fd, "!", 1) == 1, "append write");
    check(fd_read(0, fd, buffer, 1) == -(i32)E_BADF, "read on O_WRONLY fails");
    fd_close(0, fd);
    check(do_stat("/usr/hello", 0, &stat) == E_OK && stat.size == 11, "append grew file");

    check(fd_open(0, "/usr", VFS_O_RDWR, 0) == -(i32)E_ISDIR, "open directory for write fails");
    check(fd_open(0, "/usr/hello", VFS_O_RDONLY | VFS_O_DIRECTORY, 0) == -(i32)E_NOTDIR, "O_DIRECTORY on file fails");
    check(fd_open(0, "/usr/absent", VFS_O_RDONLY, 0) == -(i32)E_NOENT, "open missing fails");

    const dcache_stats_s* stats = dcache_stats();
    u32 misses = stats->misses;
    check(fd_open(0, "/usr/absent", VFS_O_RDONLY, 0) == -(i32)E_NOENT, "open missing again fails");
    check(stats->misses == misses, "negative dentry cached");
}

static void check_sparse(void) {
    vfs_stat_s stat;
    i64 position = -1;

    i32 fd = fd_open(0, "/usr/sparse", VFS_O_CREAT | VFS_O_RDWR, 0644);
    check(fd >= 0, "open sparse");
    check(fd_seek(0, fd, 300000, VFS_SEEK_SET, &position) == E_OK, "seek past end");
    check(fd_write(0, fd, "tail", 4) == 4, "write beyond hole");
    check(fd_stat(0, fd, &stat) == E_OK && stat.size == 300004 && stat.blocks == 1, "sparse size and blocks");
    check(fd_seek(0, fd, 150000, VFS_SEEK_SET, &position) == E_OK, "seek into hole");
    buffer[0] = 0xFF;
    buffer[1] = 0xFF;
    check(fd_read(0, fd, buffer, 2) == 2 && buffer[0] == 0 && buffer[1] == 0, "hole reads zeros");
    fd_close(0, fd);
}

static void check_links(void) {
    vfs_stat_s stat;
    check(do_link("/usr/hello", "/usr/hola") == E_OK, "link");
    check(do_stat("/usr/hola", 0, &stat) == E_OK && stat.nlink == 2, "nlink after link");
    check(do_link("/usr", "/usrcopy") == -(i32)E_PERM, "link directory fails");
    check(do_unlink("/usr/hello") == E_OK, "unlink original");
    check(do_stat("/usr/hello", 0, &stat) == -(i32)E_NOENT, "original gone");
    check(do_stat("/usr/hola", 0, &stat) == E_OK && stat.nlink == 1, "nlink after unlink");

    i32 fd = fd_open(0, "/usr/hola", VFS_O_RDONLY, 0);
    check(fd >= 0 && fd_read(0, fd, buffer, 16) == 11 && memcmp(buffer, "hello, vfs!", 11) == 0, "content survives via link");
    fd_close(0, fd);
}

static void check_symlinks(void) {
    vfs_stat_s stat;
    char content[32];

    check(do_symlink("/usr/hola", "/alias") == E_OK, "symlink");
    check(do_stat("/alias", 0, &stat) == E_OK && VFS_ISREG(stat.mode), "stat follows symlink");
    check(do_stat("/alias", VFS_O_NOFOLLOW, &stat) == E_OK && VFS_ISLNK(stat.mode), "lstat sees symlink");
    check(do_readlink("/alias", content, sizeof(content)) == 9 && memcmp(content, "/usr/hola", 9) == 0, "readlink");

    i32 fd = fd_open(0, "/alias", VFS_O_RDONLY, 0);
    check(fd >= 0 && fd_read(0, fd, buffer, 16) == 11, "open through symlink");
    fd_close(0, fd);
    check(fd_open(0, "/alias", VFS_O_RDONLY | VFS_O_NOFOLLOW, 0) == -(i32)E_LOOP, "O_NOFOLLOW fails on symlink");

    check(do_symlink("/loopb", "/loopa") == E_OK, "symlink loop half");
    check(do_symlink("/loopa", "/loopb") == E_OK, "symlink loop whole");
    check(fd_open(0, "/loopa", VFS_O_RDONLY, 0) == -(i32)E_LOOP, "symlink loop detected");

    check(do_symlink("hola", "/usr/rel") == E_OK, "relative symlink");
    fd = fd_open(0, "/usr/rel", VFS_O_RDONLY, 0);
    check(fd >= 0 && fd_read(0, fd, buffer, 16) == 11, "open through relative symlink");
    fd_close(0, fd);

    check(do_symlink("/usr", "/dirlink") == E_OK, "directory symlink");
    check(do_stat("/dirlink/hola", 0, &stat) == E_OK, "walk through directory symlink");

    check(
        do_unlink("/alias") == E_OK && do_unlink("/loopa") == E_OK &&
        do_unlink("/loopb") == E_OK && do_unlink("/usr/rel") == E_OK &&
        do_unlink("/dirlink") == E_OK, "unlink symlinks"
    );
}

static void check_rename(void) {
    vfs_stat_s stat;
    check(do_rename("/usr/hola", "/usr/world") == E_OK, "rename");
    check(do_stat("/usr/hola", 0, &stat) == -(i32)E_NOENT, "old name gone");
    check(do_stat("/usr/world", 0, &stat) == E_OK, "new name present");
    check(do_rename("/usr/world", "/tmp/world") == E_OK, "cross-directory rename");
    check(do_stat("/tmp/world", 0, &stat) == E_OK && stat.size == 11, "moved file intact");

    check(do_mkdir("/tmp/dir", 0755) == E_OK, "mkdir rename victim");
    check(do_rename("/tmp/world", "/tmp/dir") == -(i32)E_ISDIR, "file over directory fails");
    check(do_rename("/tmp/dir", "/tmp/world") == -(i32)E_NOTDIR, "directory over file fails");
    check(do_rename("/usr", "/usr/share/usr") == -(i32)E_INVAL, "rename into own subtree fails");

    i32 fd = fd_open(0, "/tmp/victim", VFS_O_CREAT | VFS_O_WRONLY, 0644);
    check(fd >= 0 && fd_close(0, fd) == E_OK, "create rename victim file");
    check(do_rename("/tmp/world", "/tmp/victim") == E_OK, "rename over existing file");
    check(do_stat("/tmp/victim", 0, &stat) == E_OK && stat.size == 11, "replaced content");
    check(do_rmdir("/tmp/dir") == E_OK, "rmdir");
}

static void check_readdir(void) {
    vfs_dirent_s entry;
    u32 count = 0, seen = 0;
    i32 ret;

    i32 fd = fd_open(0, "/usr", VFS_O_RDONLY | VFS_O_DIRECTORY, 0);
    check(fd >= 0, "open directory");
    while ((ret = fd_readdir(0, fd, &entry)) == 1) {
        count++;
        if (strcmp(entry.name, ".") == 0)
            seen |= 1;
        if (strcmp(entry.name, "..") == 0)
            seen |= 2;
        if (strcmp(entry.name, "share") == 0 && entry.type == VFS_DT_DIR)
            seen |= 4;
        if (strcmp(entry.name, "local") == 0 && entry.type == VFS_DT_DIR)
            seen |= 8;
        if (strcmp(entry.name, "sparse") == 0 && entry.type == VFS_DT_REG)
            seen |= 16;
    }

    check(ret == 0 && count == 5 && seen == 31, "readdir enumerates entries");
    fd_close(0, fd);

    fd = fd_open(0, "/usr/sparse", VFS_O_RDONLY, 0);
    check(fd >= 0 && fd_readdir(0, fd, &entry) == -(i32)E_NOTDIR, "readdir on file fails");
    fd_close(0, fd);

    check(do_rmdir("/usr") == -(i32)E_NOTEMPTY, "rmdir non-empty fails");
}

static void check_mounts(void) {
    vfs_stat_s stat, other;
    check(do_mkdir("/mnt", 0755) == E_OK, "mkdir mountpoint");
    check(do_mount("none", "/mnt", "tmpfs", 0) == E_OK, "mount");
    check(do_mount("none", "/mnt/absent", "tmpfs", 0) == -(i32)E_NOENT, "mount on missing dir fails");
    check(do_mount("none", "/mnt", "nofs", 0) == -(i32)E_NODEV, "unknown fstype fails");

    i32 fd = fd_open(0, "/mnt/inner", VFS_O_CREAT | VFS_O_WRONLY, 0644);
    check(fd >= 0 && fd_write(0, fd, "abc", 3) == 3, "create across mountpoint");
    check(do_stat("/mnt", 0, &stat) == E_OK && do_stat("/", 0, &other) == E_OK && stat.dev != other.dev, "mount has own device");
    check(do_stat("/mnt/../usr", 0, &stat) == E_OK, "dotdot escapes mount");
    check(do_umount("/mnt") == -(i32)E_BUSY, "umount busy with open file");
    fd_close(0, fd);
    check(do_umount("/mnt") == E_OK, "umount");
    check(do_stat("/mnt/inner", 0, &stat) == -(i32)E_NOENT, "unmounted content gone");
    check(do_stat("/mnt", 0, &stat) == E_OK && stat.dev == other.dev, "mountpoint restored");
    check(do_umount("/usr") == -(i32)E_INVAL, "umount non-mount fails");
    check(do_umount("/") == -(i32)E_BUSY, "umount root fails");
}

static void check_truncate(void) {
    vfs_stat_s stat;
    check(do_truncate("/tmp/victim", 3) == E_OK, "truncate shrink");
    check(do_stat("/tmp/victim", 0, &stat) == E_OK && stat.size == 3 && stat.blocks == 1, "shrunk size");
    check(do_truncate("/tmp/victim", 100) == E_OK, "truncate extend");

    i32 fd = fd_open(0, "/tmp/victim", VFS_O_RDONLY, 0);
    check(
        fd >= 0 &&
        fd_read(0, fd, buffer, 200) == 100 &&
        memcmp(buffer, "hel", 3) == 0
        && buffer[3] == 0
        && buffer[99] == 0, "truncate zero fill"
    );
    fd_close(0, fd);

    check(do_truncate("/usr", 0) == -(i32)E_ISDIR, "truncate directory fails");
    check(do_truncate("/tmp/victim", -1) == -(i32)E_INVAL, "negative length fails");
}

static void check_bigfile(void) {
    vfs_stat_s stat;
    i64 position = -1;
    for (u32 i = 0; i < sizeof(pattern); i++)
        pattern[i] = (u8)((i * GOLDEN_RATIO) >> 24);

    i32 fd = fd_open(0, "/tmp/big", VFS_O_CREAT | VFS_O_RDWR, 0644);
    check(fd >= 0, "open big");
    check(fd_write(0, fd, pattern, sizeof(pattern)) == (i32)sizeof(pattern), "multi-page write");
    check(fd_seek(0, fd, 0, VFS_SEEK_SET, &position) == E_OK, "rewind big");
    memset(buffer, 0, sizeof(buffer));
    check(
        fd_read(0, fd, buffer, sizeof(buffer)) == (i32)sizeof(buffer) &&
        memcmp(buffer, pattern, sizeof(pattern)) == 0, "multi-page read"
    );
    check(fd_stat(0, fd, &stat) == E_OK && stat.blocks == sizeof(pattern) / PG_SZ, "block accounting");
    fd_close(0, fd);
    check(do_unlink("/tmp/big") == E_OK, "unlink big");
}

static void check_ext2(void) {
    vfs_stat_s stat;
    i64 position = -1;

    check(do_mkdir("/disk", 0755) == E_OK, "ext2 mkdir mountpoint");
    if (do_mount("ata0", "/disk", "ext2", 0) != E_OK) {
        dprintf("[VFS] Skipping ext2 self-test, no disk\n");
        do_rmdir("/disk");
        return;
    }

    check(do_mkdir("/disk/etc", 0755) == E_OK, "ext2 mkdir");
    i32 fd = fd_open(0, "/disk/etc/motd", VFS_O_CREAT | VFS_O_RDWR, 0644);
    check(fd >= 0, "ext2 create");
    check(fd_write(0, fd, "hello, ext2", 11) == 11, "ext2 write");
    check(fd_seek(0, fd, 0, VFS_SEEK_SET, &position) == E_OK, "ext2 rewind");
    check(fd_read(0, fd, buffer, 16) == 11 && memcmp(buffer, "hello, ext2", 11) == 0, "ext2 read back");
    fd_close(0, fd);

    fd = fd_open(0, "/disk/etc/big", VFS_O_CREAT | VFS_O_RDWR, 0644);
    check(fd >= 0 && fd_write(0, fd, pattern, sizeof(pattern)) == (i32)sizeof(pattern), "ext2 indirect write");
    check(fd_seek(0, fd, 0, VFS_SEEK_SET, &position) == E_OK, "ext2 rewind big");
    memset(buffer, 0, sizeof(buffer));
    check(
        fd_read(0, fd, buffer, sizeof(buffer)) == (i32)sizeof(buffer) &&
        memcmp(buffer, pattern, sizeof(pattern)) == 0, "ext2 indirect read"
    );
    fd_close(0, fd);

    check(do_link("/disk/etc/motd", "/disk/etc/motd2") == E_OK, "ext2 link");
    check(do_stat("/disk/etc/motd2", 0, &stat) == E_OK && stat.nlink == 2, "ext2 nlink");
    check(do_symlink("/disk/etc/motd", "/disk/alias") == E_OK, "ext2 symlink");
    check(do_stat("/disk/alias", 0, &stat) == E_OK && stat.size == 11, "ext2 stat through symlink");
    check(do_rename("/disk/etc/motd", "/disk/motd") == E_OK, "ext2 rename");

    vfs_statfs_s statfs;
    check(do_statfs("/disk", &statfs) == E_OK && statfs.bfree < statfs.blocks, "ext2 statfs");
    check(do_sync() == E_OK, "ext2 sync");

    check(do_unlink("/disk/alias") == E_OK, "ext2 unlink symlink");
    check(do_unlink("/disk/motd") == E_OK && do_unlink("/disk/etc/motd2") == E_OK, "ext2 unlink");
    check(do_umount("/disk") == E_OK, "ext2 umount");

    check(do_mount("ata0", "/disk", "ext2", 0) == E_OK, "ext2 remount");
    check(do_stat("/disk/etc/big", 0, &stat) == E_OK && stat.size == (i64)sizeof(pattern), "ext2 persistence");
    check(do_unlink("/disk/etc/big") == E_OK, "ext2 cleanup big");
    check(do_rmdir("/disk/etc") == E_OK, "ext2 cleanup dir");
    check(do_umount("/disk") == E_OK, "ext2 final umount");
    check(do_rmdir("/disk") == E_OK, "ext2 cleanup mountpoint");
}

void vfs_selftest(void) {
    check_directories();
    check_files();
    check_sparse();
    check_links();
    check_symlinks();
    check_rename();
    check_readdir();
    check_mounts();
    check_truncate();
    check_bigfile();
    check_ext2();

    const dcache_stats_s* stats = dcache_stats();
    dprintf("[VFS] Self-test: %u passed, %u failed\n", passed, failed);
    dprintf("[VFS] dcache: %u hits, %u misses, %u allocated, %u pruned\n", stats->hits, stats->misses, stats->allocated, stats->pruned);
    dprintf("[VFS] heap: %u of %u pages free\n", heap_pages_free(), HEAP_ARENA_PG);
}
