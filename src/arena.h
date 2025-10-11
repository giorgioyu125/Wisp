/*
 * @file 
 */
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

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


void   arena_reset(Arena* arena);

#endif
