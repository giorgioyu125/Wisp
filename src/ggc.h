/**
 * @file ggc.h
 * @brief Generational Garbage Collector for LISP DSL
 * @details Implements a copying GC for nursery and mark-compact for old generation
 */
#ifndef GGC_H
#define GGC_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "vec.h"

/* -------------------- Constants and Configuration -------------------- */

#define DEFAULT_ALIGN 16
#define FORWARDED_TAG 255

/* -------------------- Memory Management Structures -------------------- */

/**
 * @brief Represents a contiguous memory region.
 */
typedef struct MemRegion {
    char* start;    ///< Start of the memory region
    char* end;      ///< End of the memory region (exclusive)
} MemRegion;

/**
 * @brief Nursery generation with Eden and two survivor spaces.
 * @details Implements a copying garbage collector using semi-space algorithm.
 */
typedef struct Nursery {
    MemRegion eden;    ///< Eden space for new allocations
    MemRegion s0;      ///< Survivor space 0
    MemRegion s1;      ///< Survivor space 1
    
    bool to_space_is_s0;  ///< false => s0 is to-space, true => s1 is from-space
    char* bump_ptr;       ///< Current allocation pointer (Eden during mutation, to-space during GC)
} Nursery;

/**
 * @brief Old generation using bump-pointer allocation.
 * @details Long-lived objects promoted from nursery.
 */
typedef struct OldGen {
    MemRegion region;   ///< Memory region for old generation
    char* bump_ptr;     ///< Current allocation pointer
} OldGen;

/**
 * @brief Complete heap containing all memory spaces.
 */
typedef struct Heap {
    Nursery nursery;    ///< Young generation
    OldGen old_gen;     ///< Old generation
    
    void* heap_memory_block;   ///< Underlying memory block (from malloc)
    size_t heap_memory_size;   ///< Total size of memory block
} Heap;

/**
 * @brief Main garbage collector context.
 */
typedef struct Gc {
    Heap* heap;					   ///< Managed heap
    Vec* stack;					   ///< Root stack for GC scanning
    
    /* Thresholds */
    size_t nursery_alloc_threshold;    ///< Threshold to trigger minor GC
    size_t old_gen_alloc_threshold;    ///< Threshold to trigger major GC
    
    uint8_t promotion_age_threshold;   ///< Age threshold for promotion to old gen
    bool collection_in_progress;       ///< Flag to prevent recursive collection
} Gc;



/* -------------------- Root Management -------------------- */

/**
 * @brief Registers a root pointer with the garbage collector.
 * @param gc Garbage collector context
 * @param root Pointer to the root object pointer
 */
static inline void ggc_add_root(Gc* gc, void* root) {
    if (!gc || !gc->stack || !root) return;
    vec_add(&gc->stack, root);
}

/* -------------------- Heap Management -------------------- */

/**
 * @brief Initializes a new GC heap.
 * @param h Pointer to heap structure to initialize
 * @param eden_bytes Size of Eden space
 * @param survivor_bytes Size of each survivor space
 * @param old_bytes Size of old generation
 * @param align Memory alignment (0 = use default)
 * @return 0 on success, -1 on failure
 */
int heap_init(Heap* h, size_t eden_bytes, size_t survivor_bytes, size_t old_bytes, size_t align);

/**
 * @brief Destroys a heap and frees all memory.
 * @param h Pointer to heap to destroy
 */
void heap_destroy(Heap* h);

/**
 * @brief Checks if a pointer points to an object in the nursery.
 * @param gc Garbage collector context
 * @param ptr Pointer to check
 * @return true if pointer is in nursery (Eden or survivor spaces)
 */
static inline bool is_in_nursery(Gc* gc, void* ptr) {
    if (!ptr) return false;
    
    Nursery* n = &gc->heap->nursery;
    char* p = (char*)ptr;
    
    bool in_eden = (p >= n->eden.start && p < n->eden.end);
    bool in_s0 = (p >= n->s0.start && p < n->s0.end);
    bool in_s1 = (p >= n->s1.start && p < n->s1.end);
    
    return in_eden || in_s0 || in_s1;
}

/* -------------------- Utility Functions -------------------- */

/**
 * @brief Aligns a pointer up to the specified alignment.
 * @param p Pointer to align
 * @param a Alignment (must be power of 2)
 * @return Aligned pointer
 */
static inline char* align_up_ptr(char* p, size_t a) {
    uintptr_t x = (uintptr_t)p;
    return (char*)((x + (a - 1)) & ~(a - 1));
}

/**
 * @brief Sets up a memory region.
 * @param r Pointer to memory region
 * @param base Base address
 * @param size Size in bytes
 */
static inline void memregion_init(MemRegion* r, char* base, size_t size) {
    r->start = base;
    r->end = base + size;
}

/**
 * @brief Gets the size of a memory region.
 * @param r Pointer to memory region
 * @return Size in bytes
 */
static inline size_t memregion_size(const MemRegion* r) {
    return (size_t)(r->end - r->start);
}

/* ------------------ GC core functions ------------------ */


// GC Configuration
#ifndef GGC_EDEN_SIZE
#define GGC_EDEN_SIZE      (2 * 1024 * 1024)   // 1MB
#endif

#ifndef GGC_SURVIVOR_SIZE  
#define GGC_SURVIVOR_SIZE  (1 * 1024 * 1024)   // 1MB
#endif

#ifndef GGC_OLD_GEN_SIZE
#define GGC_OLD_GEN_SIZE   (2 * 1024 * 1024)   // 2MB
#endif

#ifndef GGC_STACK_SIZE
#define GGC_STACK_SIZE     1024                // 1K objects
#endif

#ifndef GGC_STACK_LEN
#define GGC_STACK_LEN      MINIMUM_STACK_LEN    // 1K objects
#endif

/**
 * @brief Initializes garbage collector with heap and symbol table.
 * @param gc Garbage collector context to initialize
 * @param heap Initialized heap
 * @return 0 on success, -1 on failure
 */
Gc* ggc_init(void);

/**
 * @brief Destroys garbage collector and cleans up resources.
 * @param gc Garbage collector context to destroy
 */
void ggc_destroy(Gc* gc);

/* --------------------- GGC Collection and Garbage ------------------- */

/**
 * @brief Free only one element in the stack of the Gc.
 * @param gc Garbage collector context
 */
int ggc_free_slot(Gc* gc, size_t index);

/**
 * @brief Performs a minor garbage collection on the nursery.
 * @param gc Garbage collector context
 */
void ggc_minor_collect(Gc* gc);

/**
 * @brief Performs a major garbage collection on the old generation.
 * @param gc Garbage collector context
 */
void ggc_major_collect(Gc* gc);


#endif /* GGC_H */
