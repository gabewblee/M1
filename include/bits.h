#pragma once

#include <config.h>

#define BIT(x)              (1u << (x))                    /* Builds a bitmask with x-th bit set         */
#define BIT_TO_WORD(x)      ((x) >> 5)                     /* Converts a bit index to a word index       */
#define WORD_TO_BIT(x)      ((x) << 5)                     /* Converts a word index to a bit index       */
#define ALIGN_UP_TO(x, a)   (((x) + (a) - 1) & ~((a) - 1)) /* Rounds x up to the nearest multiple of a   */
#define ALIGN_DOWN_TO(x, a) ((x) & ~((a) - 1))             /* Rounds x down to the nearest multiple of a */
#define ARRAY_SZ(a)         (sizeof(a) / sizeof((a)[0]))   /* Element count of array a                   */
