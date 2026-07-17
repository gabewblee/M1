#pragma once

#include <uapi/types.h>

#define HEAP_ARENA_PG 128u        /* Arena size in pages         */
#define HEAP_ALIGN    8u          /* Minimum object alignment    */
#define HEAP_LARGEST  2048u       /* Largest size-class object   */
#define SHRINKER_MAX  4u          /* Registered shrinker limit   */
#define SHRINK_ALL    0xFFFFFFFFu /* Reclaim as much as possible */

typedef struct cache_s {
    const char* name;     /* Cache name for diagnostics    */
    u32         size;     /* Aligned object size in bytes  */
    u32         total;    /* Objects carved from the arena */
    u32         used;     /* Objects currently allocated   */
    void*       freelist; /* Free objects, linked in place */
} cache_s;

typedef u32 (*shrinker_f)(u32 count);

/**
 * heap_init - Initializes the kmalloc-style size-class caches.
 */
void heap_init(void);

/**
 * pg_alloc - Allocates one page, invoking registered shrinkers under pressure.
 * Returns: The new page, or NULL when out of memory.
 */
void* pg_alloc(void);

/**
 * pg_free - Returns @page to the page allocator.
 * @page: The page to return.
 */
void pg_free(void* page);

/**
 * cache_init - Initializes @cache to serve objects of @size bytes.
 * @cache: The cache to initialize.
 * @name: The cache name, used for diagnostics.
 * @size: The object size in bytes.
 */
void cache_init(cache_s* cache, const char* name, u32 size);

/**
 * cache_alloc - Allocates one object from @cache, or NULL when out of memory.
 * @cache: The cache to allocate from.
 * Returns: The new object, or NULL when out of memory.
 */
void* cache_alloc(cache_s* cache);

/**
 * cache_free - Returns @object to @cache.
 * @cache: The owning cache.
 * @object: The object to return.
 */
void cache_free(cache_s* cache, void* object);

/**
 * heap_alloc - Allocates @size bytes from the best-fit size class, or NULL.
 * @size: The number of bytes to allocate.
 * Returns: The new allocation, or NULL when out of memory.
 */
void* heap_alloc(u32 size);

/**
 * heap_free - Returns the @size-byte allocation @object to its size class.
 * @object: The allocation to return.
 * @size: The allocation size in bytes.
 */
void heap_free(void* object, u32 size);

/**
 * shrinker_register - Registers @shrinker to run when the arena is exhausted.
 * @shrinker: The shrinker callback to register.
 */
void shrinker_register(shrinker_f shrinker);

/**
 * heap_pages_free - Returns the number of pages still available.
 * Returns: The number of pages still available.
 */
u32 heap_pages_free(void);
