#include <uapi/errno.h>
#include <userspace/libc/minmax.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/vfs.h>
#include <userspace/shell/builtins.h>
#include <userspace/shell/line.h>

#define COPY_CHUNK 512u

static const char* error_name(i32 code) {
    switch (-code) {
        case E_INVAL:    return "invalid argument";
        case E_NOMEM:    return "out of memory";
        case E_NOSYS:    return "not supported";
        case E_PERM:     return "not permitted";
        case E_NOENT:    return "no such file or directory";
        case E_EXIST:    return "file exists";
        case E_NOTDIR:   return "not a directory";
        case E_ISDIR:    return "is a directory";
        case E_NOTEMPTY: return "directory not empty";
        case E_BADF:     return "bad file descriptor";
        case E_MFILE:    return "too many open files";
        case E_LOOP:     return "too many symbolic links";
        case E_NAMELONG: return "name too long";
        case E_XDEV:     return "cross-device link";
        case E_NODEV:    return "no such device";
        case E_BUSY:     return "resource busy";
        case E_NOSPC:    return "no space left on device";
        case E_IO:       return "input/output error";
        default:         return "unknown error";
    }
}

static i32 report(const char* name, const char* subject, i32 code) {
    if (code < 0)
        printf("%s: %s: %s\n", name, subject, error_name(code));

    return code;
}

static i32 resolve(shell_state_s* shell, const char* input, char* output) {
    i32 ret = path_resolve(shell->cwd, input, output, PATH_MAX);
    if (ret != E_OK)
        printf("shell: %s: %s\n", input, error_name(ret));

    return ret;
}

static i32 parse_u32(const char* text, u32* result) {
    u32 value = 0;
    if (*text == '\0')
        return -(i32)E_INVAL;

    for (; *text; text++) {
        if (*text < '0' || *text > '9')
            return -(i32)E_INVAL;

        value = value * 10u + (u32)(*text - '0');
    }

    *result = value;
    return E_OK;
}

static i32 do_help(shell_state_s* shell, u32 argc, char** argv) {
    builtin_list();
    return E_OK;
}

static i32 do_clear(shell_state_s* shell, u32 argc, char** argv) {
    return clear();
}

static i32 do_echo(shell_state_s* shell, u32 argc, char** argv) {
    u32 words = argc;
    u32 append = 0;
    const char* target = NULL;
    if (argc >= 3 && (strcmp(argv[argc - 2u], ">") == 0 || strcmp(argv[argc - 2u], ">>") == 0)) {
        append = argv[argc - 2u][1] == '>';
        target = argv[argc - 1u];
        words  = argc - 2u;
    }

    char text[LINE_MAX];
    u32 len = 0;
    for (u32 i = 1; i < words; i++) {
        u32 wlen = (u32)strlen(argv[i]);
        if (len + wlen + 2u > sizeof(text))
            break;

        if (i > 1)
            text[len++] = ' ';

        memcpy(text + len, argv[i], wlen);
        len += wlen;
    }

    text[len++] = '\n';
    if (!target) {
        write(text, len);
        return E_OK;
    }

    char path[PATH_MAX];
    i32 ret = resolve(shell, target, path);
    if (ret != E_OK)
        return ret;

    u32 flags = VFS_O_WRONLY | VFS_O_CREAT | (append ? VFS_O_APPEND : VFS_O_TRUNC);
    i32 fd = vfs_open(path, flags, 0644);
    if (fd < 0)
        return report("echo", target, fd);

    ret = vfs_write(fd, text, len);
    vfs_close(fd);
    return (ret < 0) ? report("echo", target, ret) : E_OK;
}

static i32 do_pwd(shell_state_s* shell, u32 argc, char** argv) {
    printf("%s\n", shell->cwd);
    return E_OK;
}

static i32 do_cd(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, (argc > 1) ? argv[1] : "/", path);
    if (ret != E_OK)
        return ret;

    vfs_stat_s stat;
    ret = vfs_stat(path, 0, &stat);
    if (ret != E_OK)
        return report("cd", path, ret);

    if (!VFS_ISDIR(stat.mode))
        return report("cd", path, -(i32)E_NOTDIR);

    strlcpy(shell->cwd, path, sizeof(shell->cwd));
    return E_OK;
}

static char type_char(u32 type) {
    switch (type) {
        case VFS_DT_DIR:
            return 'd';
        case VFS_DT_LNK:
            return 'l';
        default:
            return '-';
    }
}

static i32 do_ls(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, (argc > 1) ? argv[1] : ".", path);
    if (ret != E_OK)
        return ret;

    i32 fd = vfs_open(path, VFS_O_RDONLY | VFS_O_DIRECTORY, 0);
    if (fd < 0)
        return report("ls", path, fd);

    vfs_dirent_s entry;
    while ((ret = vfs_readdir(fd, &entry)) == 1) {
        char child[PATH_MAX];
        vfs_stat_s stat = {0};
        if (path_resolve(path, entry.name, child, sizeof(child)) == E_OK)
            vfs_stat(child, VFS_O_NOFOLLOW, &stat);

        printf("%c %8d %s\n", type_char(entry.type), (i32)stat.size, entry.name);
    }

    vfs_close(fd);
    return (ret < 0) ? report("ls", path, ret) : E_OK;
}

static i32 do_cat(shell_state_s* shell, u32 argc, char** argv) {
    for (u32 i = 1; i < argc; i++) {
        char path[PATH_MAX];
        i32 ret = resolve(shell, argv[i], path);
        if (ret != E_OK)
            return ret;

        i32 fd = vfs_open(path, VFS_O_RDONLY, 0);
        if (fd < 0)
            return report("cat", argv[i], fd);

        char buffer[COPY_CHUNK];
        while ((ret = vfs_read(fd, buffer, sizeof(buffer))) > 0)
            write(buffer, (u32)ret);

        vfs_close(fd);
        if (ret < 0)
            return report("cat", argv[i], ret);
    }

    return E_OK;
}

static const char* type_name(u32 mode) {
    if (VFS_ISDIR(mode))
        return "directory";

    if (VFS_ISLNK(mode))
        return "symbolic link";

    return "regular file";
}

static i32 do_stat(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    vfs_stat_s stat;
    ret = vfs_stat(path, VFS_O_NOFOLLOW, &stat);
    if (ret != E_OK)
        return report("stat", argv[1], ret);

    printf("  File: %s\n", path);
    printf("  Type: %s\n", type_name(stat.mode));
    printf("  Size: %d  Blocks: %u  Block size: %u\n",
           (i32)stat.size, stat.blocks, stat.blocksize);
    printf(" Inode: %u  Links: %u  Device: %u\n", stat.ino, stat.nlink, stat.dev);
    return E_OK;
}

static i32 do_mkdir(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    return report("mkdir", argv[1], vfs_mkdir(path, 0755));
}

static i32 do_rmdir(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    return report("rmdir", argv[1], vfs_rmdir(path));
}

static i32 do_rm(shell_state_s* shell, u32 argc, char** argv) {
    for (u32 i = 1; i < argc; i++) {
        char path[PATH_MAX];
        i32 ret = resolve(shell, argv[i], path);
        if (ret != E_OK)
            return ret;

        ret = report("rm", argv[i], vfs_unlink(path));
        if (ret != E_OK)
            return ret;
    }

    return E_OK;
}

static i32 do_mv(shell_state_s* shell, u32 argc, char** argv) {
    char from[PATH_MAX], to[PATH_MAX];
    i32 ret = resolve(shell, argv[1], from);
    if (ret == E_OK)
        ret = resolve(shell, argv[2], to);

    if (ret != E_OK)
        return ret;

    return report("mv", argv[1], vfs_rename(from, to));
}

static i32 do_cp(shell_state_s* shell, u32 argc, char** argv) {
    char from[PATH_MAX], to[PATH_MAX];
    i32 ret = resolve(shell, argv[1], from);
    if (ret == E_OK)
        ret = resolve(shell, argv[2], to);

    if (ret != E_OK)
        return ret;

    i32 src = vfs_open(from, VFS_O_RDONLY, 0);
    if (src < 0)
        return report("cp", argv[1], src);

    i32 dst = vfs_open(to, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0644);
    if (dst < 0) {
        vfs_close(src);
        return report("cp", argv[2], dst);
    }

    char buffer[COPY_CHUNK];
    for (;;) {
        i32 got = vfs_read(src, buffer, sizeof(buffer));
        if (got <= 0) {
            ret = got;
            break;
        }

        i32 put = vfs_write(dst, buffer, (u32)got);
        if (put != got) {
            ret = (put < 0) ? put : -(i32)E_IO;
            break;
        }
    }

    vfs_close(src);
    vfs_close(dst);
    return report("cp", argv[1], ret);
}

static i32 do_ln(shell_state_s* shell, u32 argc, char** argv) {
    u32 symbolic = strcmp(argv[1], "-s") == 0;
    if (argc != 3u + symbolic) {
        printf("usage: ln [-s] <target> <name>\n");
        return -(i32)E_INVAL;
    }

    char name[PATH_MAX];
    i32 ret = resolve(shell, argv[2u + symbolic], name);
    if (ret != E_OK)
        return ret;

    if (symbolic)
        return report("ln", argv[3], vfs_symlink(argv[2], name));

    char target[PATH_MAX];
    ret = resolve(shell, argv[1], target);
    if (ret != E_OK)
        return ret;

    return report("ln", argv[2], vfs_link(target, name));
}

static i32 do_readlink(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    char content[PATH_MAX];
    ret = vfs_readlink(path, content, sizeof(content) - 1u);
    if (ret < 0)
        return report("readlink", argv[1], ret);

    content[ret] = '\0';
    printf("%s\n", content);
    return E_OK;
}

static i32 do_touch(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    i32 fd = vfs_open(path, VFS_O_WRONLY | VFS_O_CREAT, 0644);
    if (fd < 0)
        return report("touch", argv[1], fd);

    vfs_close(fd);
    return E_OK;
}

static i32 do_truncate(shell_state_s* shell, u32 argc, char** argv) {
    u32 length;
    if (parse_u32(argv[2], &length) != E_OK) {
        printf("usage: truncate <path> <length>\n");
        return -(i32)E_INVAL;
    }

    char path[PATH_MAX];
    i32 ret = resolve(shell, argv[1], path);
    if (ret != E_OK)
        return ret;

    return report("truncate", argv[1], vfs_truncate(path, (i64)length));
}

static i32 do_df(shell_state_s* shell, u32 argc, char** argv) {
    char path[PATH_MAX];
    i32 ret = resolve(shell, (argc > 1) ? argv[1] : ".", path);
    if (ret != E_OK)
        return ret;

    vfs_statfs_s statfs;
    ret = vfs_statfs(path, &statfs);
    if (ret != E_OK)
        return report("df", path, ret);

    printf("%s: %u/%u blocks free, %u/%u inodes free, block size %u\n",
           path, statfs.bfree, statfs.blocks, statfs.ffree, statfs.files,
           statfs.blocksize);
    return E_OK;
}

static i32 do_sync(shell_state_s* shell, u32 argc, char** argv) {
    return report("sync", "all", vfs_sync());
}

static i32 do_mount(shell_state_s* shell, u32 argc, char** argv) {
    char target[PATH_MAX];
    i32 ret = resolve(shell, argv[3], target);
    if (ret != E_OK)
        return ret;

    return report("mount", argv[3], vfs_mount(argv[2], target, argv[1], 0));
}

static i32 do_umount(shell_state_s* shell, u32 argc, char** argv) {
    char target[PATH_MAX];
    i32 ret = resolve(shell, argv[1], target);
    if (ret != E_OK)
        return ret;

    return report("umount", argv[1], vfs_umount(target));
}

static const builtin_s builtins[] = {
    { "help",     "",                            "List available commands",       1, 1,             do_help     },
    { "clear",    "",                            "Clear the screen",              1, 1,             do_clear    },
    { "echo",     "[text] [> file | >> file]",   "Print text or write to a file", 1, SHELL_ARG_MAX, do_echo     },
    { "pwd",      "",                            "Print the working directory",   1, 1,             do_pwd      },
    { "cd",       "[dir]",                       "Change the working directory",  1, 2,             do_cd       },
    { "ls",       "[dir]",                       "List a directory",              1, 2,             do_ls       },
    { "cat",      "<file>...",                   "Print file contents",           2, SHELL_ARG_MAX, do_cat      },
    { "stat",     "<path>",                      "Show file attributes",          2, 2,             do_stat     },
    { "mkdir",    "<dir>",                       "Create a directory",            2, 2,             do_mkdir    },
    { "rmdir",    "<dir>",                       "Remove an empty directory",     2, 2,             do_rmdir    },
    { "rm",       "<file>...",                   "Remove files",                  2, SHELL_ARG_MAX, do_rm       },
    { "mv",       "<from> <to>",                 "Move or rename an entry",       3, 3,             do_mv       },
    { "cp",       "<from> <to>",                 "Copy a file",                   3, 3,             do_cp       },
    { "ln",       "[-s] <target> <name>",        "Create a hard or soft link",    3, 4,             do_ln       },
    { "readlink", "<link>",                      "Print a symlink target",        2, 2,             do_readlink },
    { "touch",    "<file>",                      "Create an empty file",          2, 2,             do_touch    },
    { "truncate", "<file> <length>",             "Resize a file",                 3, 3,             do_truncate },
    { "df",       "[path]",                      "Show filesystem usage",         1, 2,             do_df       },
    { "sync",     "",                            "Flush filesystem caches",       1, 1,             do_sync     },
    { "mount",    "<fstype> <source> <target>",  "Mount a filesystem",            4, 4,             do_mount    },
    { "umount",   "<target>",                    "Unmount a filesystem",          2, 2,             do_umount   },
};

const builtin_s* builtin_find(const char* name) {
    for (u32 i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++)
        if (strcmp(builtins[i].name, name) == 0)
            return &builtins[i];

    return NULL;
}

static void print_padded(const char* text, u32 width) {
    u32 len = (u32)strlen(text);
    write(text, len);
    for (u32 i = len; i < width; i++)
        putc(' ');
}

void builtin_list(void) {
    for (u32 i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        print_padded(builtins[i].name, 10);
        print_padded(builtins[i].usage, 29);
        printf("%s\n", builtins[i].help);
    }
}
