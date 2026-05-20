global _start
_start:
    xor ebp, ebp

    extern main
    call main

    push eax

    extern sys_thread_exit
    call sys_thread_exit

.hang:
    hlt
    jmp .hang
