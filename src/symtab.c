/*
 * @file symtab.c (Generational Garbage Collector)
 * @brief Simple implementation of a Garbage Collector for Wisp.
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "symtab.h"
#include "vec.h"


/* -------------------- Symbol Table Lifetime -------------------- */

SymTab* symtab_create(size_t initial_capacity) {
    return vec_new(initial_capacity, sizeof(Symbol*));
}

void symtab_destroy(SymTab** st) {
	vec_free(st);
}

/* -------------------- Symbol Table API (Main Interface) -------------------- */


Symbol* symtab_lookup(SymTab* st, const char* name) {
    uint32_t sym_hash = hash(name);
    
    for (size_t i = 0; i < st->elem_num; i++) {
        Symbol* sym = vec_at(st, i);
        if (sym->hash == sym_hash 
			&& strcmp(sym->name, name) == 0) {
            return sym;
        }
    }

    return NULL;
}
