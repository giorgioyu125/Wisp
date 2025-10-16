/*
 * @file arena.c
 */
#include <stdlib.h>
#include <stdint.h>

#include "arena.h"

#define ARENA_ALIGNMENT sizeof(void*)
#define DEFAULT_ARENA_SIZE 1 * 1024

Arena* arena_create(size_t capacity) {
    Arena* arena = malloc(sizeof(Arena));
    if (!arena) return NULL;

    arena->start = malloc(capacity);
    if (!arena->start) {
        free(arena);
        return NULL;
    }

    arena->capacity = capacity;
    arena->offset = 0;
    return arena;
}

void arena_free(Arena* arena) {
    if (!arena) return;
    free(arena->start);
    free(arena);
}

void* arena_alloc(Arena** arena_ptr, size_t size) {
    if (!arena_ptr || !*arena_ptr || size == 0) {
        return NULL;
    }

    Arena* current_arena = *arena_ptr;

    #define ARENA_ALIGNMENT sizeof(void*)
    uintptr_t current_ptr = (uintptr_t)current_arena->start + current_arena->offset;
    uintptr_t aligned_ptr = (current_ptr + ARENA_ALIGNMENT - 1) & ~(ARENA_ALIGNMENT - 1);
    size_t aligned_offset = aligned_ptr - (uintptr_t)current_arena->start;

    if (aligned_offset + size > current_arena->capacity) {
        size_t new_capacity = (size > DEFAULT_ARENA_SIZE) ? size : DEFAULT_ARENA_SIZE;

        Arena* new_arena = arena_create(new_capacity);
        if (!new_arena) {
            return NULL;
        }
        current_arena->next = new_arena;

        *arena_ptr = new_arena;

        void* ptr = new_arena->start;
        new_arena->offset = size;
        return ptr;
    }

    void* ptr = current_arena->start + aligned_offset;
    current_arena->offset = aligned_offset + size;
    return ptr;
}

void arena_reset(Arena* arena) {
    if (arena) {
        arena->offset = 0;
    }
}
