/*
 * @file ll.h
 */
#ifndef LL_H
#define LL_H

#include "arena.h"   ///< Allocator

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * @struct This is the main struct of the Linked List data structures.
 */
typedef struct Node {
    struct Node* cdr;
    size_t size;
    _Alignas(max_align_t) unsigned char car[];
} Node;

/*
 * @struct This holds the end and start of a Linked List.
 */
typedef struct LinkedList {
    Node* head;
    size_t length;
} LinkedList;

/* -------------- API ------------- */

static inline void ll_init(LinkedList* ll) {
    ll->head = NULL;
    ll->length = 0;
}

static inline Node* node_new(const void* src, size_t size, Arena** arena) {
    Node* n = arena_alloc(arena, sizeof(Node) + size);
    if (!n) return NULL;
    n->cdr = NULL;
    n->size = size;
    if (src && size) memcpy(n->car, src, size);
    return n;
}

static inline bool ll_empty(const LinkedList* ll) {
    return ll->length == 0;
}

static inline void ll_push_front(LinkedList* ll, Node* n) {
    n->cdr = ll->head;
    ll->head = n;
    ll->length++;
}

static inline void ll_push_back(LinkedList* ll, Node* n) {
    Node* temp = ll->head;
    while (temp && temp->size == ll->length - 1) {
        temp = temp->cdr;
    }
    temp->cdr = n;
    n->cdr = NULL;
    ll->length++;
}

static inline Node* ll_pop_front(LinkedList* ll) {
    Node* n = ll->head;
    ll->head = n->cdr;
    ll->length--;
    n->cdr = NULL;
    return n;
}

static inline Node* ll_pop_back(LinkedList* ll) {
    if (ll_empty(ll)) return NULL;
    
    Node* temp = ll->head;
    while (temp->cdr && temp->cdr->size == ll->length - 2) {
        temp = temp->cdr;
    }
    Node* n = temp->cdr;
    temp->cdr = NULL;
    ll->length--;
    return n;
}

static inline void ll_remove(LinkedList* ll, Node* n) {
    Node* temp = ll->head;
    while (temp && temp != n) {
        temp = temp->cdr;
    }
    if (!temp) return;

    ll->head = n->cdr;
    ll->length--;
    n->cdr = NULL;
}

#endif
