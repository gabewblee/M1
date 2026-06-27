#pragma once

#include <compiler.h>

typedef void (*initcall_f)(void); /* Initialization function */

#define INITCALL_EARLY  0u /* Level 0: Console, IDT, and PIC       */
#define INITCALL_CORE   1u /* Level 1: PMM, VMM, and kheap         */
#define INITCALL_SUBSYS 2u /* Level 2: IPC, sched, and IRQ         */
#define INITCALL_LATE   3u /* Level 3: task0, thread0, and servers */
#define __initcall(fn, lvl)                 \
    static const initcall_f __initcall_##fn \
        __attribute__((__used__, __section__(".initcall" #lvl ".init"))) = (fn)

#define early_initcall(fn)  __initcall(fn, 0)
#define core_initcall(fn)   __initcall(fn, 1)
#define subsys_initcall(fn) __initcall(fn, 2)
#define late_initcall(fn)   __initcall(fn, 3)

/**
 * initcalls - Calls registered initcalls in level order.
 */
void __init initcalls(void);
