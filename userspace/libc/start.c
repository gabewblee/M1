#include <uapi/exec.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>

extern int main(int argc, char** argv);

/**
 * __libc_start - C entry point of every user process.
 *
 * Tokenizes the command line page the loader mapped at EXEC_ARGV_VADDR into
 * argc/argv and hands control to main. Boot servers start with an empty page,
 * so they see argc == 0 and their main(void) definitions ignore the extra
 * words pushed by this cdecl call. Should main return, the exit notification
 * wakes the spawner; for boot servers the slot is empty and the signal is a
 * harmless no-op.
 */
void __noreturn __libc_start(void) {
    static char  cmdline[EXEC_CMDLINE_MAX];
    static char* argv[EXEC_ARG_MAX + 1u];

    strlcpy(cmdline, (const char*)EXEC_ARGV_VADDR, sizeof(cmdline));
    int argc = (int)strsplit(cmdline, argv, EXEC_ARG_MAX);
    argv[argc] = NULL;

    i32 code = main(argc, argv);
    sys_signal(SERVICE_CPTR_EXIT);
    sys_thread_exit(code);
    while (1);
}
