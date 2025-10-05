/**
 * @file vec.h
 * @brief A simple vector-like library that support stack operations
 */
#ifndef VEC_H
#define VEC_H

#include <stddef.h>


#ifndef GROWTH_FACTOR
#define GROWTH_FACTOR 2  ///> Default: 2
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
Vec* vec_new(size_t elem_size, size_t initial_capacity);


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
 * @brief Remove the last element from a vector without retrieving it.
 * 
 * @details Removes the last element from the vector by decrementing the element
 *          count. The memory is optionally cleared but remains allocated for 
 *          future use. This is an O(1) operation.
 * 
 * @param[in,out] v_ptr Pointer to the vector to modify.
 * 
 * @return 0 on success, negative error code on failure:
 *         - `-1`: NULL vector pointer or the Vec is empty
 * 
 * @note The vector's capacity remains unchanged. The memory is available
 *       for reuse on the next push operation.
 * 
 * @warning This function does not return the popped value. Use vec_pop_get()
 *          if you need to retrieve the element before removal.
 * 
 * @see vec_pop_get() for retrieving the element value
 */
int vec_pop_discard(Vec* v_ptr);

/**
 * @brief Remove the last element from a vector and copy its value.
 * 
 * @details Removes the last element from the vector by decrementing the element
 *          count, optionally copying the element's value to the provided buffer
 *          before removal. This is an O(1) operation.
 * 
 * @param[in,out] v_ptr     Pointer to the vector to modify.
 * @param[out]    out_value Buffer to receive the popped element's value.
 *                          Can be NULL if the value is not needed.
 *                          Must be at least v_ptr->elem_size bytes.
 * 
 * @return 0 on success, negative error code on failure:
 *         - `-1`: NULL vector pointer  
 *         - `-2`: Vector is empty (no elements to pop)
 *         - `-3`: Invalid parameters (e.g., vector not initialized)
 * 
 * @note If out_value is NULL, behaves identically to vec_pop_discard().
 * @note The caller must ensure out_value points to sufficient memory
 *       (at least v_ptr->elem_size bytes) if not NULL.
 * 
 * @warning The out_value buffer must be properly aligned for the element type.
 *          For example, if storing pointers, it must be aligned for pointer access.
 * 
 * @see vec_pop_discard() for discarding without retrieval
 * @see vec_peek() for accessing without removal
 */
int vec_pop_get(Vec* v_ptr, void* out_value);

/**
 * @brief Get pointer to the last element without removing it.
 * 
 * @param v_ptr Vector to peek into.
 * @return Pointer to last element, or NULL if empty/invalid.
 * 
 * @note The returned pointer is valid until the next vector operation.
 */
void* vec_peek(Vec* v_ptr);


/**
 * @brief Finds the first occurrence of a value in a vector.
 *
 * This optimized version uses specialization for common element sizes (1, 2, 4, 8 bytes)
 * to replace the generic memcmp with faster, direct integer comparisons. For all other
 * sizes, it falls back to the reliable memcmp.
 *
 * The `restrict` keyword is a hint to the compiler that the memory pointed to by
 * v_ptr and value will not overlap, allowing for more aggressive optimizations.
 *
 * @param v_ptr A constant pointer to the Vec structure.
 * @param value A constant pointer to the value to search for.
 * @return A pointer to the first matching element, or NULL if not found or on error.
 */
void *vec_find(const Vec *restrict v_ptr, const void *restrict value);


/**
 * @brief Copy the last element into a buffer without removing it.
 * 
 * @param v_ptr     Vector to peek into.
 * @param out_value Buffer to receive the element (must be elem_size bytes).
 * @return 0 on success, -1 on error (NULL/empty vector or NULL buffer).
 * 
 * @code
 * int value;
 * if (vec_peek_get(&stack, &value) == 0) {
 *     // value now contains copy of top element
 * }
 * @endcode
 */
int vec_peek_get(Vec* v_ptr, void* out_value);


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
 * @brief Copy a whole vector in a new memory location. 
 *
 * @param v_ptr Vector to copy
 *
 * @return A pointer to the new Vec.
 * @retval NULL if the operation failed.
 */
Vec* vec_dup(const Vec* v_ptr);

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
