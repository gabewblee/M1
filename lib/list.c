#include "list.h"

list_node_t* list_get_front(list_node_t* head) {
    return head->next;
}

int list_is_empty(const list_node_t* head) {
    return head->next == head;
}

int list_is_attached(const list_node_t* node) {
    return node->next != node;
}

void list_add_to_head(list_node_t* node, list_node_t* head) {
    head->next->prev = node;
    node->next = head->next;
    node->prev = head;
    head->next = node;
}

void list_add_to_tail(list_node_t* node, list_node_t* head) {
    head->prev->next = node;
    node->prev = head->prev;
    node->next = head;
    head->prev = node;
}

void list_del(list_node_t* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev       = node;
    node->next       = node;
}

void list_init(list_node_t* node) {
    node->prev = node;
    node->next = node;
}