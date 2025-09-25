/*
 * @file symtab.c (Generational Garbage Collector)
 * @brief Simple implementation of a Garbage Collector for Wisp.
*/

#include <stddef.h>

#include "symtab.h"
#include "vec.h"


/* -------------------- Symbol Table Lifetime -------------------- */

SymTab* symtab_create(size_t initial_capacity) {
    return vec_new(initial_capacity, sizeof(Symbol*));
}

void symtab_destroy(SymTab** st) {
	vec_free(st);
}
