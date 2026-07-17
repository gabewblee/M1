#include <uapi/errno.h>
#include <uapi/servers.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/vfs.h>
#include <userspace/server/server.h>
#include <userspace/shell/builtins.h>
#include <userspace/shell/line.h>

SERVER_DEF(
    SERVER_ID_shell,
    1,
    0,
    RES_SERV_EP(SERVICE_CPTR_VGA, SERVER_ID_vga),
    RES_SERV_EP(SERVICE_CPTR_KBD, SERVER_ID_keyboard),
    RES_SERV_EP(SERVICE_CPTR_VFS, SERVER_ID_vfs)
);

static shell_state_s shell = { .cwd = "/" };

static u32 tokenize(char* line, char** argv) {
    u32 argc = 0;
    while (*line && argc < SHELL_ARG_MAX) {
        while (*line == ' ')
            *line++ = '\0';

        if (!*line)
            break;

        argv[argc++] = line;
        while (*line && *line != ' ')
            line++;
    }

    if (*line)
        *line = '\0';

    return argc;
}

static void dispatch(u32 argc, char** argv) {
    const builtin_s* builtin = builtin_find(argv[0]);
    if (!builtin) {
        printf("%s: command not found\n", argv[0]);
        return;
    }

    if (argc < builtin->min || argc > builtin->max) {
        printf("usage: %s %s\n", builtin->name, builtin->usage);
        return;
    }

    builtin->run(&shell, argc, argv);
}

static void mount_disk(void) {
    i32 ret = vfs_mkdir("/disk", 0755);
    if (ret != E_OK && ret != -(i32)E_EXIST) {
        printf("[Shell] Failed to create /disk\n");
        return;
    }

    ret = vfs_mount("ata0", "/disk", "ext2", 0);
    if (ret == E_OK)
        printf("[Shell] Mounted ext2 disk on /disk\n");
    else
        printf("[Shell] No ext2 disk mounted\n");
}

int main(void) {
    printf("\nM1 shell. Type 'help' for the command list.\n");
    mount_disk();

    for (;;) {
        printf("M1:%s$ ", shell.cwd);

        char line[LINE_MAX];
        i32 len = line_read(line, sizeof(line));
        if (len <= 0)
            continue;

        char* argv[SHELL_ARG_MAX];
        u32 argc = tokenize(line, argv);
        if (argc)
            dispatch(argc, argv);
    }
}
