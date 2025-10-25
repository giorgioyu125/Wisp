/*
 * @file symtab.c
 */
#include "symtab.h"
#include <stdint.h>
#include <string.h>


uint32_t symtab_hash(const char* str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

Symtab* symtab_new(Symtab* parent, Arena** arena) {
    if (!arena || !*arena) return NULL;

    Symtab* st = (Symtab*)arena_alloc(arena, sizeof(Symtab));
    if (!st) return NULL;

    size_t initial_size = parent ? 16 : SYMTAB_INITIAL_SIZE;

    st->slots = (Symbol**)arena_alloc(arena, sizeof(Symbol*) * initial_size);
    if (!st->slots) return NULL;

    memset(st->slots, 0, sizeof(Symbol*) * initial_size);

    st->size = initial_size;
    st->count = 0;
    st->arena = arena;
    st->parent = parent;

    return st;
}

static void symtab_grow(Symtab* st) {
    if (st->count < (size_t)(st->size * SYMTAB_LOAD_FACTOR)) return;

    size_t new_size = st->size * 2;

    Symbol** new_slots = arena_alloc(st->arena, sizeof(Symbol*) * new_size);
    if (!new_slots) return;

    memset(new_slots, 0, sizeof(Symbol*) * new_size);

    for (size_t i = 0; i < st->size; i++) {
        Symbol* chain = st->slots[i];
        while (chain) {
            Symbol* next = chain->next;


            size_t new_idx = chain->hash % new_size;
            chain->next = new_slots[new_idx];
            new_slots[new_idx] = chain;

            chain = next;
        }
    }


    st->slots = new_slots;
    st->size = new_size;
}

Symbol* symtab_define(Symtab* st, const char* name, Cons* value, bool is_const) {
    if (!st || !name) return NULL;

    uint32_t hash = symtab_hash(name);
    size_t idx = hash % st->size;

    Symbol* current = st->slots[idx];
    while (current) {
        if (current->hash == hash &&
            strcmp(name, current->name) == 0){
            if (current->is_const) {
                return NULL;
            }
            current->value = value;
            current->is_const = is_const;
            return current;
        }
        current = current->next;
    }


    Symbol* sym = arena_alloc(st->arena, sizeof(Symbol));
    if (!sym) return NULL;

    size_t name_len = strlen(name) + 1;
    char* interned_name = arena_alloc(st->arena, name_len);
    if (!interned_name) return NULL;
    memcpy(interned_name, name, name_len);

    sym->name = interned_name;
    sym->value = value;
    sym->hash = hash;
    sym->is_const = is_const;


    sym->next = st->slots[idx];
    st->slots[idx] = sym;

    st->count++;

    symtab_grow(st);

    return sym;
}

Symbol* symtab_lookup(const Symtab* st, const char* name) {
    if (!st || !name) return NULL;

    uint32_t hash = symtab_hash(name);

    const Symtab* current_st = st;
    while (current_st){
        size_t idx = hash % current_st->size;
        Symbol* current = current_st->slots[idx];
        while (current) {
            if (current->hash == hash &&
                (strcmp(name, current->name) == 0)){
                return current;
            }
            current = current->next;
        }
        current_st = current_st->parent;
    }
    return NULL;
}

Symbol* symtab_lookup_local(const Symtab* st, const char* name) {
    if (!st || !name) return NULL;

    uint32_t hash = symtab_hash(name);
    size_t idx = hash % st->size;

    Symbol* current = st->slots[idx];
    while (current) {
        if (current->hash == hash && 
            (strcmp(name, current->name) == 0)){
                return current;
        }
        current = current->next;
    }

    return NULL;
}

int symtab_set(Symtab* st, const char* name, Cons* value) {
    if (!st || !name || !value) return -1;

    Symbol* sym = symtab_lookup(st, name);
    if (!sym) return -1;
    if (sym->is_const) return -2;

    sym->value = value;
    return 0;
}
