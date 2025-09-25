/*
 * @file ggc.c (Generational Garbage Collector)
 * @brief Simple implementation of a Garbage Collector for Wisp.
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ggc.h"
#include "vec.h"
#include "symtab.h"

/* -------------- CONFIGURATION ------------- */ 

#define GGC_ZERO_MEMORY 1  ///> This defines if the alloc is zero-initialized or not


/* ---------------- HEAP --------------- */


#define DEFAULT_ALIGN 16
int heap_init(Heap* h, size_t eden_bytes, size_t survivor_bytes, size_t old_bytes, size_t align) {
    if (!h) return -1;

    if (align == 0) align = DEFAULT_ALIGN;
    if ((align & (align - 1)) != 0) align = DEFAULT_ALIGN;

    size_t total_size = eden_bytes + (2 * survivor_bytes) + old_bytes + align;

    void* raw = malloc(total_size);
    if (!raw) return -1;

    h->heap_memory_block = raw;
    h->heap_memory_size  = total_size;

    uintptr_t mask = (uintptr_t)(align - 1);
    char* p = (char*)(((uintptr_t)raw + mask) & ~mask);

    /* Nursery - Eden */
	memregion_init(&h->nursery.eden, p, eden_bytes);
	p += eden_bytes;

    /* Nursery - Survivor 0 */
	memregion_init(&h->nursery.s0, p, survivor_bytes);
	p += survivor_bytes;

    /* Nursery - Survivor 1 */
	memregion_init(&h->nursery.s1, p, survivor_bytes);
	p += survivor_bytes;

    /* Old Generation */
	memregion_init(&h->old_gen.region, p, old_bytes);

    /* bump pointers e stato iniziale */
    h->nursery.bump_ptr     = h->nursery.eden.start;
    h->nursery.to_space_is_s0 = true;
    h->old_gen.bump_ptr     = h->old_gen.region.start;

    return 0;
}

void heap_destroy(Heap* h) {
    if (!h) return;

    if (h->heap_memory_block) {
        free(h->heap_memory_block);
    }

    h->heap_memory_block = NULL;
    h->heap_memory_size  = 0;

    h->nursery.eden.start = h->nursery.eden.end = NULL;
    h->nursery.s0.start   = h->nursery.s0.end   = NULL;
    h->nursery.s1.start   = h->nursery.s1.end   = NULL;
    h->nursery.bump_ptr   = NULL;
    h->nursery.to_space_is_s0 = false;

    h->old_gen.region.start = h->old_gen.region.end = NULL;
    h->old_gen.bump_ptr     = NULL;
}


/* ------------------ GC core functions ------------------ */

Gc* gc_init(void) {
    Gc* gc = malloc(sizeof(Gc));
    if (!gc) {
        perror("ggc_init: Failed to allocate GC structure");
        return NULL;
    }
    
    gc->heap = malloc(sizeof(Heap));
    if (!gc->heap) {
        perror("ggc_init: Failed to allocate heap structure");
        free(gc);
        return NULL;
    }
    
    if (heap_init(gc->heap, GGC_EDEN_SIZE, GGC_SURVIVOR_SIZE, 
                  GGC_OLD_GEN_SIZE, DEFAULT_ALIGN) < 0) {
        perror("ggc_init: Failed to initialize heap");
        free(gc->heap);
        free(gc);
        return NULL;
    }
    
    gc->stack = vec_new(GGC_STACK_SIZE, sizeof(void*));
    if (!gc->stack) {
        perror("ggc_init: Failed to allocate root stack");
        heap_destroy(gc->heap);
        free(gc->heap);
        free(gc);
        return NULL;
    }
    
    gc->nursery_alloc_threshold = GGC_EDEN_SIZE / 2;
    gc->old_gen_alloc_threshold = GGC_OLD_GEN_SIZE / 2;
    gc->promotion_age_threshold = 3;
    gc->collection_in_progress = false;
    
    return gc;
}

void gc_destroy(Gc* gc) {
    if (!gc) return;
    
    if (gc->stack) {
		vec_free(&gc->stack);
    }
    
    if (gc->heap) {
        heap_destroy(gc->heap);
        free(gc->heap);
    }
    
    free(gc);
}

/* --------------------- GGC Collection and Garbage ------------------- */

int gc_free_slot(Gc* gc, size_t index) {
    if (!gc || !gc->stack) return -1;
    if (index >= gc->stack->elem_count) return -1;
    
    return vec_rem(gc->stack, index);
}


/* -------------------- Functions to Copy Symbols -------------------- */ 


void* gc_copy_value_data(Gc* gc, void* src_data, TypeTag type, void* to_space) {
	switch (type) {
		case T_NIL:{
			
		}

		case T_CONS:{

		}

		case T_SYMBOL:{

		}

		case T_INT:{

		}

		case T_STRING:{

		}

		case T_FLOAT:{

		}

		case T_BOOL:{

		}

		case T_VECTOR:{

		}

		case T_FUNCTION:{

		}

		case T_MACRO: {

		}

		case T_SPECIAL_FORM:{

		}


	}
}

Symbol* gc_copy_symbol(Gc* gc, Symbol* src, char* to_space) {

}

/* ------------------------- Garbage collection ------------------- */

void gc_major_collect(Gc *gc) {

}

void gc_minor_collect(Gc* gc) {

}
