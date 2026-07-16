#include <uapi/uapi.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/heap.h>
#include <userspace/vfs/radix.h>

typedef struct radix_node_s {
    u32   count;             /* Populated slot count */
    void* slots[RADIX_SLOTS]; /* Children or items    */
} radix_node_s;

static cache_s radix_node_cache;

static inline u32 radix_slot(u32 index, u32 level) {
    return (index >> ((level - 1u) * RADIX_BITS)) & (RADIX_SLOTS - 1u);
}

static inline int radix_covers(u32 height, u32 index) {
    u32 bits = height * RADIX_BITS;
    return bits >= 32u || index < (1u << bits);
}

static radix_node_s* node_alloc(void) {
    radix_node_s* node = cache_alloc(&radix_node_cache);
    if (node)
        memset(node, 0, sizeof(*node));

    return node;
}

void radix_tree_init(radix_s* tree) {
    tree->root   = NULL;
    tree->height = 0;
}

i32 radix_insert(radix_s* tree, u32 index, void* item) {
    if (!item)
        return -(i32)E_INVAL;

    if (!tree->root) {
        tree->root = node_alloc();
        if (!tree->root)
            return -(i32)E_NOMEM;

        tree->height = 1;
    }

    while (!radix_covers(tree->height, index)) {
        radix_node_s* node = node_alloc();
        if (!node)
            return -(i32)E_NOMEM;

        node->slots[0] = tree->root;
        node->count    = 1;
        tree->root     = node;
        tree->height++;
    }

    radix_node_s* node = tree->root;
    for (u32 level = tree->height; level > 1; level--) {
        u32 slot = radix_slot(index, level);
        radix_node_s* child = node->slots[slot];
        if (!child) {
            child = node_alloc();
            if (!child)
                return -(i32)E_NOMEM;

            node->slots[slot] = child;
            node->count++;
        }

        node = child;
    }

    u32 slot = radix_slot(index, 1);
    if (node->slots[slot])
        return -(i32)E_EXIST;

    node->slots[slot] = item;
    node->count++;
    return E_OK;
}

void* radix_lookup(radix_s* tree, u32 index) {
    if (!tree->root || !radix_covers(tree->height, index))
        return NULL;

    radix_node_s* node = tree->root;
    for (u32 level = tree->height; level > 1; level--) {
        node = node->slots[radix_slot(index, level)];
        if (!node)
            return NULL;
    }

    return node->slots[radix_slot(index, 1)];
}

static void* radix_erase(radix_node_s* node, u32 level, u32 index) {
    u32 slot = radix_slot(index, level);
    void* item;
    if (level == 1) {
        item = node->slots[slot];
        if (item) {
            node->slots[slot] = NULL;
            node->count--;
        }

        return item;
    }

    radix_node_s* child = node->slots[slot];
    if (!child)
        return NULL;

    item = radix_erase(child, level - 1u, index);
    if (item && child->count == 0) {
        cache_free(&radix_node_cache, child);
        node->slots[slot] = NULL;
        node->count--;
    }

    return item;
}

void* radix_delete(radix_s* tree, u32 index) {
    if (!tree->root || !radix_covers(tree->height, index))
        return NULL;

    void* item = radix_erase(tree->root, tree->height, index);
    radix_node_s* root = tree->root;
    if (item && root->count == 0) {
        cache_free(&radix_node_cache, root);
        radix_tree_init(tree);
    }

    return item;
}

static u32 radix_reap(radix_node_s* node, u32 level, u64 base, u64 start, radix_release_f release) {
    u32 count = 0;
    if (level == 1) {
        for (u32 slot = 0; slot < RADIX_SLOTS; slot++) {
            if (!node->slots[slot] || base + slot < start)
                continue;

            if (release)
                release(node->slots[slot]);

            node->slots[slot] = NULL;
            node->count--;
            count++;
        }

        return count;
    }

    u64 span = 1ull << ((level - 1u) * RADIX_BITS);
    for (u32 slot = 0; slot < RADIX_SLOTS; slot++) {
        radix_node_s* child = node->slots[slot];
        if (!child || base + (slot + 1ull) * span <= start)
            continue;

        count += radix_reap(child, level - 1u, base + slot * span, start, release);
        if (child->count == 0) {
            cache_free(&radix_node_cache, child);
            node->slots[slot] = NULL;
            node->count--;
        }
    }

    return count;
}

u32 radix_prune(radix_s* tree, u32 start, radix_release_f release) {
    if (!tree->root)
        return 0;

    u32 count = radix_reap(tree->root, tree->height, 0, start, release);
    radix_node_s* root = tree->root;
    if (root->count == 0) {
        cache_free(&radix_node_cache, root);
        radix_tree_init(tree);
    }

    return count;
}

void radix_init(void) {
    cache_init(&radix_node_cache, "radix-node", sizeof(radix_node_s));
}
