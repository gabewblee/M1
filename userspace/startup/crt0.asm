global _start
_start:
    xor ebp, ebp

    extern __libc_start
    call __libc_start

.hang:
    hlt
    jmp .hang
