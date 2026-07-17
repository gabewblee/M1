#include <userspace/libc/hash.h>
#include <userspace/libc/heap.h>
#include <userspace/libc/string.h>
#include <userspace/vfs/dcache.h>
#include <userspace/vfs/rnode.h>

#define DCACHE_BITS 8u

static hlist_head_s   buckets[1u << DCACHE_BITS];
static list_node_s    lru;
static cache_s        dentry_object_cache;
static dcache_stats_s statistics;

static hlist_head_s* d_bucket(const dentry_s* parent, u32 hash) {
    return &buckets[hash_32((u32)(uintptr_t)parent ^ hash, DCACHE_BITS)];
}

static void d_free(dentry_s* dentry) {
    if (dentry->name.name != dentry->iname)
        heap_free((void*)dentry->name.name, dentry->name.len + 1u);

    cache_free(&dentry_object_cache, dentry);
}

static dentry_s* d_kill(dentry_s* dentry) {
    dentry_s* parent = (dentry->parent == dentry) ? NULL : dentry->parent;
    if (dentry->flags & DENTRY_ONLRU)
        list_del(&dentry->lru_list_node);

    hlist_del(&dentry->hash_list_node);
    list_del(&dentry->child_list_node);
    if (dentry->node)
        rnode_put(dentry->node);

    d_free(dentry);
    return parent;
}

dentry_s* d_alloc(dentry_s* parent, const qstr_s* name) {
    dentry_s* dentry = cache_alloc(&dentry_object_cache);
    if (!dentry)
        return NULL;

    char* storage = dentry->iname;
    if (name->len >= DENTRY_INLINE) {
        storage = heap_alloc(name->len + 1u);
        if (!storage) {
            cache_free(&dentry_object_cache, dentry);
            return NULL;
        }
    }

    memcpy(storage, name->name, name->len);
    storage[name->len] = '\0';

    dentry->count                = 1;
    dentry->flags                = 0;
    dentry->node                 = NULL;
    dentry->name.name            = storage;
    dentry->name.len             = name->len;
    dentry->name.hash            = name->hash;
    dentry->hash_list_node.next  = NULL;
    dentry->hash_list_node.pprev = NULL;
    list_init(&dentry->child_list_node);
    list_init(&dentry->children);
    list_init(&dentry->lru_list_node);

    if (parent) {
        dentry->parent = dget(parent);
        dentry->sb     = parent->sb;
        list_add_to_tail(&dentry->child_list_node, &parent->children);
    } else {
        dentry->parent = dentry;
        dentry->sb     = NULL;
    }

    statistics.allocated++;
    return dentry;
}

void d_instantiate(dentry_s* dentry, rnode_s* node) {
    dentry->node = node;
}

void d_rehash(dentry_s* dentry) {
    hlist_add_head(&dentry->hash_list_node, d_bucket(dentry->parent, dentry->name.hash));
}

void d_add(dentry_s* dentry, rnode_s* node) {
    d_instantiate(dentry, node);
    d_rehash(dentry);
}

void d_drop(dentry_s* dentry) {
    hlist_del(&dentry->hash_list_node);
}

dentry_s* d_lookup(dentry_s* parent, const qstr_s* name) {
    hlist_head_s* bucket = d_bucket(parent, name->hash);
    dentry_s* dentry;
    hlist_for_each_entry(dentry, bucket, hash_list_node) {
        if (dentry->parent != parent)
            continue;

        if (dentry->name.hash != name->hash)
            continue;

        if (dentry->name.len != name->len)
            continue;

        if (memcmp(dentry->name.name, name->name, name->len))
            continue;

        statistics.hits++;
        return dget(dentry);
    }

    statistics.misses++;
    return NULL;
}

void d_delete(dentry_s* dentry) {
    if (dentry->count == 1) {
        rnode_put(dentry->node);
        dentry->node = NULL;
    } else {
        d_drop(dentry);
    }
}

void d_move(dentry_s* dentry, dentry_s* target) {
    d_drop(dentry);
    d_drop(target);

    if (dentry->name.name != dentry->iname)
        heap_free((void*)dentry->name.name, dentry->name.len + 1u);

    if (target->name.name == target->iname) {
        memcpy(dentry->iname, target->iname, target->name.len + 1u);
        dentry->name.name = dentry->iname;
    } else {
        dentry->name.name = target->name.name;
        target->name.name = target->iname;
        target->iname[0]  = '\0';
    }

    dentry->name.len  = target->name.len;
    dentry->name.hash = target->name.hash;
    target->name.len  = 0;
    target->name.hash = 0;

    if (dentry->parent != target->parent) {
        dentry_s* previous = dentry->parent;
        dentry->parent = dget(target->parent);
        list_del(&dentry->child_list_node);
        list_add_to_tail(&dentry->child_list_node, &dentry->parent->children);
        dput(previous);
    }

    d_rehash(dentry);
}

int d_is_ancestor(dentry_s* dentry, dentry_s* child) {
    for (;;) {
        if (child == dentry)
            return 1;

        if (child->parent == child)
            return 0;

        child = child->parent;
    }
}

dentry_s* dget(dentry_s* dentry) {
    if (!dentry)
        return NULL;

    if (dentry->count++ == 0 && (dentry->flags & DENTRY_ONLRU)) {
        list_del(&dentry->lru_list_node);
        dentry->flags &= ~DENTRY_ONLRU;
    }

    return dentry;
}

void dput(dentry_s* dentry) {
    while (dentry) {
        if (--dentry->count)
            return;

        if (hlist_is_hashed(&dentry->hash_list_node)) {
            list_add_to_tail(&dentry->lru_list_node, &lru);
            dentry->flags |= DENTRY_ONLRU;
            return;
        }

        dentry = d_kill(dentry);
    }
}

static u32 lru_reclaim(u32 count, const rsb_s* sb) {
    u32 freed = 0;
    list_node_s* node = lru.next;
    while (freed < count && node != &lru) {
        dentry_s* victim = list_entry(node, dentry_s, lru_list_node);
        node = node->next;
        if (sb && victim->sb != sb)
            continue;

        list_del(&victim->lru_list_node);
        victim->flags &= ~DENTRY_ONLRU;

        dentry_s* parent = d_kill(victim);
        if (parent)
            dput(parent);

        statistics.pruned++;
        freed++;

        node = lru.next;
    }

    return freed;
}

u32 dcache_shrink(u32 count) {
    return lru_reclaim(count, NULL);
}

void dcache_prune_sb(const rsb_s* sb) {
    lru_reclaim(SHRINK_ALL, sb);
}

const dcache_stats_s* dcache_stats(void) {
    return &statistics;
}

void dcache_init(void) {
    list_init(&lru);
    cache_init(&dentry_object_cache, "dentry", sizeof(dentry_s));
    shrinker_register(dcache_shrink);
}
