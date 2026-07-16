#pragma once

/*
 * Linux-style, type-checked min/max helpers.
 *
 * These are implemented as macros so callers can use them with any scalar
 * type without requiring typed function variants.
 */
#define __minmax_cmp(x, y, op)                                                    \
    ({                                                                            \
        __typeof__(x) __x = (x);                                                  \
        __typeof__(y) __y = (y);                                                  \
        (void)(&__x == &__y);                                                     \
        __x op __y ? __x : __y;                                                   \
    })

#define min(x, y) __minmax_cmp(x, y, <)
#define max(x, y) __minmax_cmp(x, y, >)
