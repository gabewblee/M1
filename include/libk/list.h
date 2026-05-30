#pragma once

#include <stddef.h>

#include "config.h"

typedef struct list_node_s {
    struct list_node_s* prev; /* Previous node in doubly-linked list */
    struct list_node_s* next; /* Next node in doubly-linked list     */
} list_node_s;

#define get_container_of(ptr, type, member)                                              \
    ((type*)((uintptr_t)(ptr) - offsetof(type, member)))

#define list_entry(ptr, type, member)                                                    \
    get_container_of(ptr, type, member)

#define list_first_entry(head, type, member)                                             \
    list_entry(((const list_node_s*)(head))->next, type, member)

#define list_for_each(pos, head)                                                         \
    for (pos = ((const list_node_s*)(head))->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)                                           \
    for (pos = list_entry(((const list_node_s*)(head))->next, __typeof__(*pos), member); \
         &pos->member != (head);                                                         \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

#define list_for_each_entry_safe(pos, tmp, head, member)                                 \
    for (pos = list_entry(((const list_node_s*)(head))->next, __typeof__(*pos), member), \
         tmp = list_entry(pos->member.next, __typeof__(*pos), member);                   \
         &pos->member != (head);                                                         \
         pos = tmp, tmp = list_entry(tmp->member.next, __typeof__(*tmp), member))

/**
 * list_is_empty - Checks if a list is empty.
 * @head: The head of the list.
 * Returns: 1 if the list is empty, 0 otherwise.
 */
static inline int list_is_empty(const list_node_s* head) {
    return head->next == head;
}

/**
 * list_is_attached - Checks if a list node is attached to a list.
 * @node: The list node to check.
 * Returns: 1 if the list node is attached, 0 otherwise.
 */
static inline int list_is_attached(const list_node_s* node) {
    return node->next != node;
}

/**
 * list_add_to_head - Adds a list node to the head of a list.
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
 * list_add_to_tail - Adds a list node to the tail of a list.
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
 * list_del - Deletes a list node.
 * @node: The list node to delete.
 */
static inline void list_del(list_node_s* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev       = node;
    node->next       = node;
}

/**
 * list_init - Initializes a list node.
 * @node: The list node to initialize.
 */
static inline void list_init(list_node_s* node) {
    node->prev = node;
    node->next = node;
}
