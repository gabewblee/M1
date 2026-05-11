#pragma once

#include <stddef.h>

#include "../config.h"

/**
 * kmalloc - Allocate a memory block
 * @sz: The size of the memory block
 * @return: The pointer to the allocated memory block
 */
 void* __hot kmalloc(size_t sz);

/**
 * kfree - Free a memory block
 * @ptr: The pointer to the memory block to free
 */
void __hot kfree(void* ptr);

/**
 * kheap_init - Initialize the kernel heap
 */
void __init kheap_init(void);