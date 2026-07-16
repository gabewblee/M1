#include <uapi/uapi.h>
#include <userspace/vfs/heap.h>

#define HEAP_CLASSES 8u

static u8 __aligned(PG_SZ) arena[VFS_HEAP_PG * PG_SZ];
static void*      freelist;
static u32        watermark;
static u32        spare;
static shrinker_f shrinkers[SHRINKER_MAX];
static u32        registered;
static cache_s    classes[HEAP_CLASSES];

static const char* names[HEAP_CLASSES] = {
    "heap-16",  "heap-32",  "heap-64",   "heap-128",
    "heap-256", "heap-512", "heap-1024", "heap-2048",
};

static void* pg_take(void) {
    if (freelist) {
        void* page = freelist;
        freelist = *(void**)page;
        spare--;
        return page;
    }

    if (watermark < VFS_HEAP_PG)
        return &arena[watermark++ * PG_SZ];

    return NULL;
}

void* pg_alloc(void) {
    void* page = pg_take();
    if (unlikely(!page)) {
        for (u32 i = 0; i < registered; i++)
            shrinkers[i](SHRINK_ALL);

        page = pg_take();
    }

    return page;
}

void pg_free(void* page) {
    *(void**)page = freelist;
    freelist = page;
    spare++;
}

void cache_init(cache_s* cache, const char* name, u32 size) {
    if (size < sizeof(void*))
        size = sizeof(void*);

    cache->name     = name;
    cache->size     = (size + HEAP_ALIGN - 1u) & ~(HEAP_ALIGN - 1u);
    cache->total    = 0;
    cache->used     = 0;
    cache->freelist = NULL;
}

static int cache_grow(cache_s* cache) {
    u8* page = pg_alloc();
    if (!page)
        return 0;

    u32 capacity = PG_SZ / cache->size;
    for (u32 i = 0; i < capacity; i++) {
        void* object = page + i * cache->size;
        *(void**)object = cache->freelist;
        cache->freelist = object;
    }

    cache->total += capacity;
    return 1;
}

void* cache_alloc(cache_s* cache) {
    if (unlikely(!cache->freelist) && !cache_grow(cache))
        return NULL;

    void* object = cache->freelist;
    cache->freelist = *(void**)object;
    cache->used++;
    return object;
}

void cache_free(cache_s* cache, void* object) {
    *(void**)object = cache->freelist;
    cache->freelist = object;
    cache->used--;
}

static u32 class_index(u32 size) {
    if (size <= 16u)
        return 0;

    return (32u - (u32)__builtin_clz(size - 1u)) - 4u;
}

void* heap_alloc(u32 size) {
    if (size == 0)
        return NULL;

    if (size > HEAP_LARGEST)
        return size <= PG_SZ ? pg_alloc() : NULL;

    return cache_alloc(&classes[class_index(size)]);
}

void heap_free(void* object, u32 size) {
    if (!object)
        return;

    if (size > HEAP_LARGEST)
        pg_free(object);
    else
        cache_free(&classes[class_index(size)], object);
}

void shrinker_register(shrinker_f shrinker) {
    if (registered < SHRINKER_MAX)
        shrinkers[registered++] = shrinker;
}

u32 heap_pages_free(void) {
    return (VFS_HEAP_PG - watermark) + spare;
}

void heap_init(void) {
    for (u32 i = 0; i < HEAP_CLASSES; i++)
        cache_init(&classes[i], names[i], 16u << i);
}
