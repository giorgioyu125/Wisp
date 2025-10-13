/*
 * @file 
 */
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include "vec.h"

typedef struct Arena {
    char*  start;
    size_t capacity;
    size_t offset;

    struct Arena* next;
} Arena;


/*
 *
 *
 */
Arena* arena_create(size_t capacity);


/*
 *
 *
 */
void   arena_free(Arena* arena);


/**
 * @brief Allocate memory in chain of Arenas.
 * @param arena_ptr Pointer to a pointer to the current arena.
 *                  This is updated if a new arena is created.
 * @param size      The amount of memory to allocate.
 * @return          A pointer to the allocated memory, or NULL on error.
 */
void* arena_alloc(Arena** arena_ptr, size_t size);



/* ----------------- Macro for Vec* in Arenas ---------- */

static inline Vec* arena_vec_new(Arena** a, size_t elem_size, size_t capacity) {
    if (!a || !*a) return NULL;
    size_t total = sizeof(Vec) + elem_size * capacity;
    Vec* v = (Vec*)arena_alloc(a, total);
    if (!v) return NULL;
    v->elem_num  = 0;
    v->elem_size = elem_size;
    v->maxcap    = capacity;
    v->bump_ptr  = (char*)v + sizeof(Vec);
    return v;
}
static inline int vec_push_nogrow(Vec* v_ptr, const void* elem) {
    if (!v_ptr || !elem) return -1;
    if (v_ptr->elem_num >= v_ptr->maxcap) return -2;
    memcpy(v_ptr->bump_ptr, elem, v_ptr->elem_size);
    v_ptr->bump_ptr += v_ptr->elem_size;
    v_ptr->elem_num++;
    return 0;
}
#define VEC_TOP_PTR(v, T) ((T*)((v)->bump_ptr - (v)->elem_size))
#define VEC_POP(v) do { (v)->elem_num--; (v)->bump_ptr -= (v)->elem_size; } while(0)

void   arena_reset(Arena* arena);

#endif
