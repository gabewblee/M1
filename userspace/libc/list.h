#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct list_node_s {
    struct list_node_s* prev; /* Previous node in doubly-linked list */
    struct list_node_s* next; /* Next node in doubly-linked list     */
} list_node_s;

typedef struct hlist_node_s {
    struct hlist_node_s*  next;  /* Next node in hash chain           */
    struct hlist_node_s** pprev; /* Address of the link pointing here */
} hlist_node_s;

typedef struct hlist_head_s {
    hlist_node_s* first; /* First node in hash chain */
} hlist_head_s;

#define get_container_of(ptr, type, member)                                        \
    ((type*)((uintptr_t)(ptr) - offsetof(type, member)))

#define list_entry(ptr, type, member)                                              \
    get_container_of(ptr, type, member)

#define list_first_entry(head, type, member)                                       \
    list_entry(((list_node_s*)(head))->next, type, member)

#define list_for_each(pos, head)                                                   \
    for (pos = ((list_node_s*)(head))->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)                                     \
    for (pos = list_entry(((list_node_s*)(head))->next, __typeof__(*pos), member); \
         &pos->member != (head);                                                   \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

#define list_for_each_entry_safe(pos, tmp, head, member)                           \
    for (pos = list_entry(((list_node_s*)(head))->next, __typeof__(*pos), member), \
         tmp = list_entry(pos->member.next, __typeof__(*pos), member);             \
         &pos->member != (head);                                                   \
         pos = tmp, tmp = list_entry(tmp->member.next, __typeof__(*tmp), member))

#define hlist_entry(ptr, type, member)                                             \
    get_container_of(ptr, type, member)

#define hlist_for_each_entry(pos, head, member)                                    \
    for (pos = (head)->first                                                       \
             ? hlist_entry((head)->first, __typeof__(*pos), member)                \
             : NULL;                                                               \
         pos;                                                                      \
         pos = pos->member.next                                                    \
             ? hlist_entry(pos->member.next, __typeof__(*pos), member)             \
             : NULL)

/**
 * list_is_empty - Checks if a list is empty.
 * @head: The head of the list.
 * Returns: 1 if the list is empty, 0 otherwise.
 */
static inline int list_is_empty(list_node_s* head) {
    return head->next == head;
}

/**
 * list_is_attached - Checks if @node is attached to a list.
 * @node: The list node to check.
 * Returns: 1 if the list node is attached, 0 otherwise.
 */
static inline int list_is_attached(list_node_s* node) {
    return node->next != node;
}

/**
 * list_add_to_head - Adds @node to the head of a list.
 * @node: The list node to add.
 * @head: The head of the list.
 */
static inline void list_add_to_head(list_node_s* node, list_node_s* head) {
    head->next->prev = node;
    node->next       = head->next;
    node->prev       = head;
    head->next       = node;
}

/**
 * list_add_to_tail - Adds @node to the tail of a list.
 * @node: The list node to add.
 * @head: The head of the list.
 */
static inline void list_add_to_tail(list_node_s* node, list_node_s* head) {
    head->prev->next = node;
    node->prev       = head->prev;
    node->next       = head;
    head->prev       = node;
}

/**
 * list_del - Deletes @node.
 * @node: The list node to delete.
 */
static inline void list_del(list_node_s* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev       = node;
    node->next       = node;
}

/**
 * list_init - Initializes @node.
 * @node: The list node to initialize.
 */
static inline void list_init(list_node_s* node) {
    node->prev = node;
    node->next = node;
}

/**
 * hlist_is_hashed - Checks if @node is on a hash chain.
 * @node: The hash list node to check.
 * Returns: 1 if the node is hashed, 0 otherwise.
 */
static inline int hlist_is_hashed(hlist_node_s* node) {
    return node->pprev != NULL;
}

/**
 * hlist_add_head - Adds @node to the front of the chain at @head.
 * @node: The hash list node to add.
 * @head: The head of the hash chain.
 */
static inline void hlist_add_head(hlist_node_s* node, hlist_head_s* head) {
    hlist_node_s* first = head->first;
    node->next = first;
    if (first)
        first->pprev = &node->next;

    head->first = node;
    node->pprev = &head->first;
}

/**
 * hlist_del - Unlinks @node from its chain. No-op if @node is not hashed.
 * @node: The hash list node to unlink.
 */
static inline void hlist_del(hlist_node_s* node) {
    if (!node->pprev)
        return;

    *node->pprev = node->next;
    if (node->next)
        node->next->pprev = node->pprev;

    node->next  = NULL;
    node->pprev = NULL;
}
