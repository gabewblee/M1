#pragma once

#include <stddef.h>

#include "../config.h"

typedef struct list_node_t {
    struct list_node_t* prev;
    struct list_node_t* next;
} list_node_t;

#define list_entry(ptr, type, member) get_container_of(ptr, type, member)

#define list_first_entry(head, type, member) list_entry((head)->next, type, member)

#define list_for_each(pos, head)                                                    \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)                                      \
    for (pos = list_entry((head)->next, __typeof__(*pos), member);                  \
         &pos->member != (head);                                                    \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

#define list_for_each_entry_safe(pos, tmp, head, member)                            \
    for (pos = list_entry((head)->next, __typeof__(*pos), member),                  \
         tmp = list_entry(pos->member.next, __typeof__(*pos), member);              \
         &pos->member != (head);                                                    \
         pos = tmp, tmp = list_entry(tmp->member.next, __typeof__(*tmp), member))

/**
 * list_get_front - Returns the front of a list.
 * @head: The head of the list.
 * @return: The front of the list.
 */
list_node_t* list_get_front(list_node_t* head);

/**
 * list_is_empty - Checks if a list is empty.
 * @head: The head of the list.
 * @return: 1 if the list is empty, 0 otherwise.
 */
int list_is_empty(const list_node_t* head);

/**
 * list_is_attached - Checks if a list node is attached to a list.
 * @node: The list node to check.
 * @return: 1 if the list node is attached, 0 otherwise.
 */
int list_is_attached(const list_node_t* node);

/**
 * list_add_to_head - Adds a list node to the head of a list.
 * @node: The list node to add.
 * @head: The head of the list.
 */
void list_add_to_head(list_node_t* node, list_node_t* head);

/**
 * list_add_to_tail - Adds a list node to the tail of a list.
 * @node: The list node to add.
 * @head: The head of the list.
 */
void list_add_to_tail(list_node_t* node, list_node_t* head);

/**
 * list_del - Deletes a list node.
 * @node: The list node to delete.
 */
void list_del(list_node_t* node);

/**
 * list_init - Initializes a list node.
 * @node: The list node to initialize.
 */
void list_init(list_node_t* node);
