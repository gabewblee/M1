#pragma once

#include <compiler.h>

typedef void (*initcall_f)(void);

#define __initcall(fn, lvl)                 \
    static const initcall_f __initcall_##fn \
        __attribute__((__used__, __section__(".initcall" #lvl ".init"))) = (fn)

#define arch_initcall(fn)   __initcall(fn, 0)
#define mm_initcall(fn)     __initcall(fn, 1)
#define kernel_initcall(fn) __initcall(fn, 2)
#define user_initcall(fn)   __initcall(fn, 3)

/**
 * initcalls - Calls registered initcalls in level order.
 */
void __init initcalls(void);
