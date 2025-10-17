/*
 * @file ll.h
 */
#ifndef LL_H
#define LL_H

#include "arena.h" ///< Allocator

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * @struct This is the main struct of the Linked List data structures.
 */
typedef struct Node{
    struct Node* prev;
    struct Node* next;
    size_t size;
    _Alignas(max_align_t) unsigned char data[];
}Node;

/*
 * @struct This holds the end and start of a Linked List.
 */
typedef struct LinkedList{
    Node* head;
    Node* tail;
    size_t length;
}LinkedList;

/* -------------- API ------------- */


static inline Node* node_new(const void* src, size_t size, Arena** arena) {
    Node* n = arena_alloc(arena, sizeof(Node) + size);
    if (!n) return NULL;
    n->prev = n->next = NULL;
    n->size = size;
    if (src && size) memcpy(n->data, src, size);
    return n;
}

static inline void ll_init(LinkedList* ll, Arena** arena) {
    LinkedList* result = arena_alloc(arena, ll->length);
    result->head = NULL;
    result->tail = NULL;
    result->length = ll->length;
}

static inline bool ll_empty(const LinkedList* ll) {
    return ll->length == 0;
}

static inline void ll_push_front(LinkedList* ll, Node* n) {
    n->prev = NULL;
    n->next = ll->head;
    if (ll->head) ll->head->prev = n; else ll->tail = n;
    ll->head = n;
    ll->length++;
}

static inline void ll_push_back(LinkedList* ll, Node* n) {
    n->next = NULL;
    n->prev = ll->tail;
    if (ll->tail) ll->tail->next = n; else ll->head = n;
    ll->tail = n;
    ll->length++;
}

static inline Node* ll_pop_front(LinkedList* ll) {
    Node* n = ll->head;
    if (!n) return NULL;
    ll->head = n->next;
    if (ll->head) ll->head->prev = NULL; else ll->tail = NULL;
    n->prev = n->next = NULL;
    ll->length--;
    return n;
}

static inline Node* ll_pop_back(LinkedList* ll) {
    Node* n = ll->tail;
    if (!n) return NULL;
    ll->tail = n->prev;
    if (ll->tail) ll->tail->next = NULL; else ll->head = NULL;
    n->prev = n->next = NULL;
    ll->length--;
    return n;
}

static inline void ll_remove(LinkedList* ll, Node* n) {
    if (n->prev) n->prev->next = n->next; else ll->head = n->next;
    if (n->next) n->next->prev = n->prev; else ll->tail = n->prev;
    n->prev = n->next = NULL;
    ll->length--;
}

#endif
