#pragma once

#include <uapi/types.h>

#define GOLDEN_RATIO 0x61C88647u /* 2^32 / golden ratio, prime-ish multiplier */

/**
 * hash_32 - Folds @value into @bits bucket-index bits by golden-ratio multiplication.
 * @value: The value to hash.
 * @bits: The number of bucket-index bits to produce.
 * Returns: The hashed bucket index.
 */
static inline u32 hash_32(u32 value, u32 bits) {
    return (value * GOLDEN_RATIO) >> (32u - bits);
}

/**
 * hash_ptr - Folds the pointer @ptr into @bits bucket-index bits.
 * @ptr: The pointer to hash.
 * @bits: The number of bucket-index bits to produce.
 * Returns: The hashed bucket index.
 */
static inline u32 hash_ptr(const void* ptr, u32 bits) {
    return hash_32((u32)(uintptr_t)ptr, bits);
}

/**
 * hash_partial_name - Mixes the byte @c into the running name hash @prev.
 * @c: The byte to mix in.
 * @prev: The running hash so far.
 * Returns: The updated running hash.
 */
static inline u32 hash_partial_name(u32 c, u32 prev) {
    return (prev + (c << 4) + (c >> 4)) * 11u;
}

/**
 * hash_full_name - Hashes @len bytes of the component @name in one pass.
 * @name: The component bytes to hash.
 * @len: The number of bytes to hash.
 * Returns: The name hash.
 */
static inline u32 hash_full_name(const char* name, u32 len) {
    u32 hash = 0;
    for (u32 i = 0; i < len; i++)
        hash = hash_partial_name((u8)name[i], hash);

    return hash;
}
