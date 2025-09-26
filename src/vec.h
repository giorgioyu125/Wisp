/**
 * @file vec.h
 * @brief A simple vector-like library that support stack operations
 */
#ifndef VEC_H
#define VEC_H

#include <stddef.h>


#ifndef GROWTH_FACTOR
#define GROWTH_FACTOR 2  ///> Default: double capacity
#endif

/**
 * @struct This is the main struct of the Vector implementation
 */
typedef struct Vec{
	size_t elem_num;
	size_t elem_size;
	size_t maxcap;

	char* bump_ptr;
} Vec;


/* ---------------------- VECTOR OPERATIONS ---------------------- */ 


/**
 * @brief Create a new vector whose backing storage is obtained with a single
 *        `malloc`.
 *
 * The returned pointer points to a contiguous block that contains:
 *   1. The `Vec` header.
 *   2. `maxcap * elem_size` bytes of uninitialized storage for elements.
 *
 * @param maxcap   Number of elements for which space is pre-allocated.
 * @param elem_size Size (in bytes) of a single element.
 *
 * @return Pointer to a valid `Vec` on success.
 * @retval NULL   Allocation failed (`errno` is set by `malloc`).
 */
Vec* vec_new(size_t maxcap, size_t elem_size);


/**
 * @brief Append an element to the end of the vector.
 *
 * The caller **must** ensure the vector is not already full
 * (`v_ptr->elem_num < v_ptr->maxcap`).
 *
 * @param v_ptr  Pointer to a pointer to an existing vector.
 * @param value  Pointer to the element to be copied into the vector.
 *
 * @retval 0  Success.
 * @retval -1 `v_ptr` is NULL.
 * @retval -2 Vector is full (`elem_num == maxcap`).
 */
int vec_push(Vec** v_ptr, const void* value);


/**
 * @brief Delete the **first** occurrence of an element whose bytes match
 *        `value`, using `memcmp` with `elem_size` bytes.
 *
 * Remaining elements are shifted left one position.
 *
 * @param v_ptr  Pointer to an existing vector.
 * @param value  Pointer to a value that will be compared against every stored
 *               element.
 *
 * @retval 0  Element found and removed.
 * @retval -1 `v_ptr` is NULL.
 * @retval -2 Vector is empty.
 * @retval -3 Element not found.
 */
int vec_del(Vec *v_ptr, const void *value);


/**
 * @brief Remove **all** occurrences of elements that byte-wise equal `value`,
 *        using `memcmp` with `elem_size` bytes.
 *
 * Remaining elements are compacted so the vector remains dense.
 *
 * @param v_ptr  Pointer to an existing vector.
 * @param value  Pointer to a value used for comparison.
 *
 * @return Number of elements actually removed.
 * @retval -1 `v_ptr` is NULL.
 */
int vec_rem(Vec *v_ptr, const void *value);


/**
 * @brief Reduce the pre-allocated capacity of the vector to `newcap`.
 *
 * @param v_ptr  Pointer to a pointer to an existing vector.
 * @param newcap Desired new capacity (in elements).
 *
 * @retval 0   Capacity successfully updated.
 * @retval -1  An error occurred (e.g., invalid input, allocation failure).
 */
int vec_shrink(Vec** vec, size_t newcap);


/**
 * @brief Release all resources held by the vector and set the caller’s pointer
 *        to NULL for safety.
 *
 * Equivalent to `free(*v_ptr); *v_ptr = NULL;`
 *
 * @param v_ptr  Address of a `Vec *` variable.
 *
 * @retval 0  Success.
 * @retval -1 `v_ptr` is NULL or `*v_ptr` is already NULL.
 */
int vec_free(Vec **v_ptr);



/**
 * @brief Get a pointer to the element at index `idx`.
 *
 * @param v_ptr  Vector to read from.
 * @param idx    Index (0-based).
 *
 * @return Pointer to the element on success.
 * @retval NULL If index is out of bounds.
 */
void* vec_at(const Vec* v_ptr, size_t idx);


/**
 * @brief Copy the element at index `idx` into `out`.
 *
 * @param v_ptr  Vector to read from.
 * @param idx    Index.
 * @param out    Buffer to copy into.
 *
 * @retval 0  Success.
 * @retval -1 Invalid input or out of bounds.
 */
int vec_get(const Vec* v_ptr, size_t idx, void* out);


/**
 * @brief Return number of elements currently stored.
 */
static inline size_t vec_len(const Vec* v_ptr) {
    return v_ptr ? v_ptr->elem_num : 0;
}

/**
 * @brief Return maximum number of elements the vector can hold.
 */
static inline size_t vec_capacity(const Vec* v_ptr) {
    return v_ptr ? v_ptr->maxcap : 0;
}

/**
 * @brief Remove all elements from the vector, resetting length to 0.
 *        Capacity remains unchanged.
 */
static inline int vec_clear(Vec* v_ptr) {
    if (!v_ptr) return -1;
    v_ptr->elem_num = 0;
    v_ptr->bump_ptr = (char*)(v_ptr + 1);
    return 0;
}


#endif
