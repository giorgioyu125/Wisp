/*
 * @file arean.h
 * @brief a simple arena implementation for stupid interface
 */
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <string.h>
#include "vec.h"

/*
 * @struct Arena main struct
 */
typedef struct Arena {
    char*  start;      ///< Pointer to the start of allocated memory block
    size_t capacity;   ///< Total size of the memory block in bytes
    size_t offset;     ///< Current position/offset for next allocation
    struct Arena* next; ///< Pointer to next arena in linked list (for chaining)
} Arena;

/**
 * Creates a new memory arena with the specified capacity.
 * 
 * @param capacity The size in bytes for the memory arena
 * @return Pointer to the newly created Arena, or NULL if allocation failed
 * 
 * Allocates and initializes a new memory arena that can be used for
 * efficient memory management within the specified capacity.
 */
Arena* arena_create(size_t capacity);

/**
 * Frees all memory associated with an arena and its chain.
 * 
 * @param arena Pointer to the arena to be freed
 * 
 * Deallocates the entire memory block and any chained arenas,
 * making the arena pointer invalid after this call.
 */
void arena_free(Arena* arena);

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
#define VEC_REPLACE(v, idx, new_val) do { \
    if (!(v) || (idx) >= (v)->elem_num) break; \
    void* pos = (char*)((v) + 1) + ((idx) * (v)->elem_size); \
    memcpy(pos, &(new_val), (v)->elem_size); \
} while(0)


void   arena_reset(Arena* arena);

#endif
