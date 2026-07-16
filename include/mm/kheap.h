#pragma once

#include <config.h>
#include <stddef.h>

/**
 * kzalloc - Allocates a zeroed memory block of at least @sz bytes.
 * @sz: The requested memory block size.
 * Returns: The pointer to the allocated memory block, or NULL on failure.
 */
void* kzalloc(size_t sz);

/**
 * kmalloc - Allocates a memory block of at least @sz bytes.
 * @sz: The requested memory block size.
 * Returns: The pointer to the allocated memory block, or NULL on failure.
 */
void* kmalloc(size_t sz);

/**
 * kfree - Frees the memory block found at @ptr.
 * @ptr: The pointer to the memory block to free.
 */
void kfree(void* ptr);
