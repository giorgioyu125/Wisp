/*
 * @file vec.c
 * @brief A simple vector library for Wisp.
*/
#include "vec.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -------------------------- VECTOR --------------------- */

Vec* vec_new(size_t elem_size, size_t initial_capacity) {
    size_t total_size = sizeof(Vec) + (elem_size * initial_capacity);
    Vec* v_ptr = (Vec*)malloc(total_size);

    if (!v_ptr) return NULL;

    v_ptr->elem_num = 0;
    v_ptr->elem_size = elem_size;
    v_ptr->maxcap = initial_capacity;

    v_ptr->bump_ptr = (char*)(v_ptr) + sizeof(Vec);

    return v_ptr;
}

int vec_push(Vec** v_ptr, const void* value) {
    if (!v_ptr || !*v_ptr) return -1;

    Vec* vec = *v_ptr;

    if (vec->elem_num >= vec->maxcap) {
        size_t newcap = vec->maxcap * GROWTH_FACTOR;

        if (newcap < vec->elem_num + 1) {
            newcap = vec->elem_num + 1;
        }

        size_t new_total_size = sizeof(Vec) + (vec->elem_size * newcap);

        Vec* new_vec = (Vec*)realloc(vec, new_total_size);
        if (!new_vec) return -2;

        new_vec->maxcap = newcap;
        new_vec->bump_ptr = (char*)(new_vec + 1) + (new_vec->elem_num * new_vec->elem_size);

        *v_ptr = new_vec;
    }

    Vec* final_vec = *v_ptr;
    memcpy(final_vec->bump_ptr, value, final_vec->elem_size);
    final_vec->bump_ptr += final_vec->elem_size;
    final_vec->elem_num++;

    return 0;
}

int vec_pop_discard(Vec* v_ptr) {
    if (!v_ptr || v_ptr->elem_num == 0) return -1;

    v_ptr->elem_num--;

    char* last_elem = v_ptr->bump_ptr - v_ptr->elem_size;
    memset(last_elem, 0, v_ptr->elem_size);

    v_ptr->bump_ptr -= v_ptr->elem_size;
    return 0;
}

int vec_pop_get(Vec* v_ptr, void* out_value) {
    if (!v_ptr || v_ptr->elem_num == 0) return -1;

    v_ptr->elem_num--;
    char* last_elem = v_ptr->bump_ptr - v_ptr->elem_size;
    if (out_value) {
        memcpy(out_value, last_elem, v_ptr->elem_size);
    }
    v_ptr->bump_ptr -= v_ptr->elem_size;
    memset(last_elem, 0, v_ptr->elem_size);
    return 0;
}

void* vec_peek(Vec* v_ptr) {
    if (!v_ptr || v_ptr->elem_num == 0) return NULL;

    char* last_elem = v_ptr->bump_ptr - v_ptr->elem_size;
    return last_elem;
}

int vec_peek_get(Vec* v_ptr, void* out_value) {
    if (!v_ptr || v_ptr->elem_num == 0 || !out_value) return -1;

    char* last_elem = v_ptr->bump_ptr - v_ptr->elem_size;
    void* result = memcpy(out_value, last_elem, v_ptr->elem_size);
    return result ? 0 : -1;
}

int vec_del(Vec* v_ptr, const void* value) {
	if (!v_ptr) return -1;
	if (v_ptr->elem_num <= 0) return -2;

    char* current = (char*)(v_ptr) + sizeof(Vec);
	for (size_t i = 0; i < v_ptr->elem_num; i++, current += v_ptr->elem_size) {
	    if (memcmp(current, value, v_ptr->elem_size) == 0) {
	        size_t bytes_to_move = (v_ptr->elem_num - i - 1) * v_ptr->elem_size;
	        memmove(current, current + v_ptr->elem_size, bytes_to_move);
	        v_ptr->elem_num--;
	        return 0;
	    }
	}

	return -3;
}

void* vec_find(const Vec* restrict v_ptr, const void* restrict value) {
    if (!v_ptr || !v_ptr->bump_ptr || !value || v_ptr->elem_num == 0) {
        return NULL;
    }

    const unsigned char* cur = (unsigned char*)(v_ptr) + sizeof(Vec);
    const size_t elem_size = v_ptr->elem_size;
    const size_t elem_num = v_ptr->elem_num;

    switch (elem_size) {
        case 1: {
            const uint8_t val = *(const uint8_t*)value;
            for (size_t i = 0; i < elem_num; ++i) {
                if (((const uint8_t *)cur)[i] == val) {
                    return (void *)(cur + i);
                }
            }
            return NULL;
        }
        case 2: {
            const uint16_t val = *(const uint16_t*)value;
            const uint16_t *ptr = (const uint16_t*)cur;
            for (size_t i = 0; i < elem_num; ++i) {
                if (ptr[i] == val) {
                    return (void *)(&ptr[i]);
                }
            }
            return NULL;
        }
        case 4: {
            const uint32_t val = *(const uint32_t*)value;
            const uint32_t *ptr = (const uint32_t*)cur;
            for (size_t i = 0; i < elem_num; ++i) {
                if (ptr[i] == val) {
                    return (void *)(&ptr[i]);
                }
            }
            return NULL;
        }
        case 8: {
            const uint64_t val = *(const uint64_t*)value;
            const uint64_t *ptr = (const uint64_t*)cur;
            for (size_t i = 0; i < elem_num; ++i) {
                if (ptr[i] == val) {
                    return (void *)(&ptr[i]);
                }
            }
            return NULL;
        }
    }

    const unsigned char* end = cur + elem_num * elem_size;
    for (; cur < end; cur += elem_size) {
        if (memcmp(cur, value, elem_size) == 0) {
            return (void *)cur;
        }
    }

    return NULL;
}

int vec_rem(Vec* v_ptr, const void* value) {
    if (!v_ptr) return -1;

    int count = 0;
    char* data_start = (char*)(v_ptr) + sizeof(Vec);
    size_t elem_size = v_ptr->elem_size;
    for (size_t i = 0; i < v_ptr->elem_num; ) {
        char* current = data_start + (i * elem_size);
        if (memcmp(current, value, elem_size) == 0) {
            size_t bytes_to_move = (v_ptr->elem_num - i - 1) * elem_size;
            if (bytes_to_move > 0) {
                memmove(current, current + elem_size, bytes_to_move);
            }
            v_ptr->elem_num--;
            count++;
        } else {
            i++;
        }
    }

    return count;
}


int vec_shrink(Vec** vec, size_t newcap) {
    if (!vec) return -1;

    Vec* v_ptr = *vec;
    if (!v_ptr) return -1;

    if (newcap < v_ptr->elem_num) {
        return -1;
    }

    if (newcap >= v_ptr->maxcap) {
        return 0;
    }

    size_t new_total_size = sizeof(Vec) + (v_ptr->elem_size * newcap);

    Vec* new_vec = (Vec*)realloc(v_ptr, new_total_size);
    if (!new_vec) {
        return -1;
    }

    new_vec->maxcap = newcap;
    new_vec->bump_ptr = (char*)(new_vec + 1) + (new_vec->elem_num * new_vec->elem_size);

    *vec = new_vec;

	return 0;
}


void* vec_at(const Vec* v_ptr, size_t idx) {
    if (!v_ptr || idx >= v_ptr->elem_num) return NULL;
    return (char*)(v_ptr) + sizeof(Vec) + (idx * v_ptr->elem_size);
}

int vec_get(const Vec* v_ptr, size_t idx, void* out) {
    if (!v_ptr || !out || idx >= v_ptr->elem_num) return -1;
    memcpy(out, (char*)(v_ptr) + sizeof(Vec) + (idx * v_ptr->elem_size), v_ptr->elem_size);
    return 0;
}


int vec_free(Vec** v_ptr) {
    if (!v_ptr) return -1;
    if (*v_ptr) {
        free(*v_ptr);
        *v_ptr = NULL;
    }
    return 0;
}


Vec* vec_dup(const Vec* v_ptr) {
    if (!v_ptr) return NULL;
    size_t total_size = sizeof(Vec) + v_ptr->elem_size * v_ptr->elem_num;

    Vec* new_vec = malloc(total_size);
    if (!new_vec) return NULL;

    memcpy(new_vec, v_ptr, total_size);

    return new_vec;
}
