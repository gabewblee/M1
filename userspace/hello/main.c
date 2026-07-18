#include <uapi/errno.h>
#include <userspace/libc/stdio.h>

int main(int argc, char** argv) {
    printf("Hello from a spawned process!\n");
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = %s\n", i, argv[i]);

    return E_OK;
}
