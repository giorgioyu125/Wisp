/*
 * @file ggc.c (Generational Garbage Collector)
 * @brief Simple implementation of a Garbage Collector for Wisp.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ggc.h"
#include "vec.h"

/* -------------- CONFIGURATION ------------- */

#define GGC_ZERO_MEMORY 1 ///> This defines if the alloc is zero-initialized or not

/* ---------------- HEAP --------------- */

#define DEFAULT_ALIGN 16
int heap_init(Heap *h, size_t eden_bytes, size_t survivor_bytes,
              size_t old_bytes, size_t align) {
    if (!h)
        return -1;

    if (align == 0)
        align = DEFAULT_ALIGN;
    if ((align & (align - 1)) != 0)
        align = DEFAULT_ALIGN;

    size_t total_size = eden_bytes + (2 * survivor_bytes) + old_bytes + align;

    void *raw = malloc(total_size);
    if (!raw)
      return -1;

    h->heap_memory_block = raw;
    h->heap_memory_size = total_size;

    char* p = align_up_ptr((char*)raw, align);

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
    h->nursery.bump_ptr = h->nursery.eden.start;
    h->nursery.to_space_is_s0 = true;
    h->old_gen.bump_ptr = h->old_gen.region.start;

    return 0;
}

void heap_destroy(Heap *h) {
  if (!h)
    return;

  if (h->heap_memory_block) {
    free(h->heap_memory_block);
  }

  h->heap_memory_block = NULL;
  h->heap_memory_size = 0;

  h->nursery.eden.start = h->nursery.eden.end = NULL;
  h->nursery.s0.start = h->nursery.s0.end = NULL;
  h->nursery.s1.start = h->nursery.s1.end = NULL;
  h->nursery.bump_ptr = NULL;
  h->nursery.to_space_is_s0 = false;

  h->old_gen.region.start = h->old_gen.region.end = NULL;
  h->old_gen.bump_ptr = NULL;
}

/* ------------------ GC core functions ------------------ */

Gc* gc_init(extract_reference extract_refs) {
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

    if (heap_init(gc->heap, GGC_EDEN_SIZE, GGC_SURVIVOR_SIZE, GGC_OLD_GEN_SIZE,
                  DEFAULT_ALIGN) < 0) {
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

    gc->extract_refs = extract_refs;

    return gc;
}

void gc_destroy(Gc* gc) {
    if (!gc)
      return;

    if (gc->stack) {
      vec_free(&gc->stack);
    }

    if (gc->heap) {
      heap_destroy(gc->heap);
      free(gc->heap);
    }

    free(gc);
}

/* ----------------------------- Copy Object ---------------------- */ 


void copy_obj(GCInfo* obj, char* to_space, extract_reference extract_refs) {
    size_t obj_size = obj->obj_size;

    memcpy(to_space, obj, obj_size);
    obj->forwarding_ptr = to_space;

    size_t ref_count;
    void*** reference_locations = extract_refs(to_space + sizeof(GCInfo), &ref_count);

    for (size_t i = 0; i < ref_count; i++) {
        void** ref_location = reference_locations[i];
        void* old_target = *ref_location;
        if (!old_target) continue;
        GCInfo* target_header = (GCInfo*)((char*)old_target - sizeof(GCInfo));

        if (target_header->forwarding_ptr) {
            *ref_location = target_header->forwarding_ptr + sizeof(GCInfo);
        }
    }
}

/* ------------------------- Garbage collection ------------------- */
bool is_in_from_space(Gc* gc, void* obj_body, MemRegion* from_survivor); ///< Forward declaration needed to write minor_collect

void gc_major_collect(Gc* gc) {

}

void gc_minor_collect(Gc* gc) {
    if (!gc) return;

    Nursery* nursery = &gc->heap->nursery;
    OldGen* old_gen = &gc->heap->old_gen;

    MemRegion* from_survivor;
    MemRegion* to_space;
    if (nursery->to_space_is_s0) {
        to_space = &nursery->s0;
        from_survivor = &nursery->s1;
    } else {
        to_space = &nursery->s1;
        from_survivor = &nursery->s0;
    }


    Vec* worklist = vec_new(sizeof(void*), 128);
    if (!worklist) {
        return;
    }

    


free:
    vec_free(&worklist);
}

/* -------------------------- Allcotion Helpers ----------------------- */

bool is_in_from_space(Gc* gc, void* obj_body, MemRegion* from_survivor) {
    if (!gc || !obj_body) return false;
    GCInfo* header = gc_header_from_obj(obj_body);
    char* header_addr = (char*)header;
    Nursery* n = &gc->heap->nursery;
    bool in_eden = (header_addr >= n->eden.start && header_addr < n->eden.end);
    bool in_from_sur = (header_addr >= from_survivor->start && header_addr < from_survivor->end);
    return in_eden || in_from_sur;
}


void* gc_alloc_nursery(Gc* gc, size_t size_body) {
    if (!gc || size_body == 0) return NULL;

    Nursery* n = &gc->heap->nursery;
    MemRegion* eden = &n->eden;

    size_t total = align_up_size(size_body, DEFAULT_ALIGN);
    char* p = align_up_ptr(n->bump_ptr, DEFAULT_ALIGN);

    size_t avail = (size_t)(eden->end - p);
    if (total > avail) {
        gc->collection_in_progress = true;
        gc_minor_collect(gc);
        gc->collection_in_progress = false;

        p = align_up_ptr(n->bump_ptr, DEFAULT_ALIGN);
        avail = (size_t)(eden->end - p);

        if (total > avail) {
            OldGen* old = &gc->heap->old_gen;
            char* op = align_up_ptr(old->bump_ptr, DEFAULT_ALIGN);
            size_t oavail = (size_t)(old->region.end - op);

            if (total > oavail) {
                gc->collection_in_progress = true;
                gc_major_collect(gc);
                gc->collection_in_progress = false;

                op = align_up_ptr(old->bump_ptr, DEFAULT_ALIGN);
                oavail = (size_t)(old->region.end - op);
                if (total > oavail) {
                    return NULL; // OOM
                }
            }

#if GGC_ZERO_MEMORY
            memset(op, 0, total);
#endif
            GCInfo* header = (GCInfo*)op;
            header->obj_size = total;
            header->gen = 0;
            header->age = 0;

            old->bump_ptr = op + total;
            return op + sizeof(GCInfo);
        }
    }

#if GGC_ZERO_MEMORY
    memset(p, 0, total);
#endif

    GCInfo* header = (GCInfo*)p;
    header->obj_size = total;
    header->gen = 0;
    header->age = 0;

    n->bump_ptr = p + total;

    return p + sizeof(GCInfo); // ritorna Body (Symbol*)
}


void* gc_alloc_old(Gc* gc, size_t size_body) {
    if (!gc || size_body == 0) return NULL;

    OldGen* og = &gc->heap->old_gen;

    char* p = align_up_ptr(og->bump_ptr, DEFAULT_ALIGN);
    size_t total = align_up_size(size_body, DEFAULT_ALIGN);

    if ((size_t)(og->region.end - p) < total) {
        if (!gc->collection_in_progress) {
            gc->collection_in_progress = true;
            gc_major_collect(gc);
            gc->collection_in_progress = false;
        }
        p = align_up_ptr(og->bump_ptr, DEFAULT_ALIGN);
        if ((size_t)(og->region.end - p) < total) {
            return NULL;
        }
    }

    GCInfo* header = (GCInfo*)p;
    header->obj_size = total;
    header->age = 0;
    header->gen = 0;

#if GGC_ZERO_MEMORY
    memset(p + sizeof(GCInfo), 0, total - sizeof(GCInfo));
#endif

    og->bump_ptr = p + total;
    return p + sizeof(GCInfo);
}

/* ------------------------- MAIN API ------------------- */

void* gc_alloc(Gc* gc, size_t size, extract_reference extract_refs) {

}
