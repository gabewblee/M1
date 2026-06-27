#pragma once

#define __multiboot            __attribute__((__section__(".multiboot"))) /* Places the function in the .multiboot section   */
#define __init                 __attribute__((__section__(".init")))      /* Places the function in the .init section        */
#define __init_data            __attribute__((__section__(".init")))      /* Places the data in the .init section            */

#define __noreturn             __attribute__((__noreturn__))              /* Function does not return                        */
#define __hot                  __attribute__((__hot__))                   /* Function is in a hot path                       */
#define __cold                 __attribute__((__cold__))                  /* Function is in a cold path                      */
#define __always_inline inline __attribute__((__always_inline__))         /* Function is always inlined                      */

#define __packed               __attribute__((__packed__))                /* Prevents padding                                */
#define __aligned(x)           __attribute__((__aligned__(x)))            /* Aligns up to a multiple of x bytes              */

#define likely(x)              __builtin_expect(!!(x), 1)                 /* Hints that the expression is likely to be true  */
#define unlikely(x)            __builtin_expect(!!(x), 0)                 /* Hints that the expression is likely to be false */
