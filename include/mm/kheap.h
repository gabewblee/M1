#pragma once

#include <stddef.h>

#include "config.h"

/**
 * kmalloc - Allocates a memory block of at least @sz bytes.
 * @sz: The size of the memory block to allocate.
 * @return: The pointer to the allocated memory block, or NULL on failure.
 */
void* kmalloc(size_t sz);

/**
 * kfree - Frees the memory block found at @ptr.
 * @ptr: The pointer to the memory block to free.
 */
void kfree(void* ptr);

/**
 * kheap_init - Initializes the kernel heap.
 */
void __init kheap_init(void);