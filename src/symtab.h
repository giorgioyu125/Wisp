/*
 * @file symtab.h
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "arena.h"
#include "parser.h"

#define SYMTAB_INITIAL_SIZE 64
#define SYMTAB_LOAD_FACTOR 0.75

typedef struct Symbol {
    const char* name;           ///< Symbol name (interned string)
    Cons* value;                ///< Symbol value (can be any Cons type)
    struct Symbol* next;        ///< Next symbol in hash chain (for collision resolution)
    uint32_t hash;              ///< Cached hash value
    bool is_const;              ///< Is this symbol immutable?
} Symbol;

typedef struct Symtab {
    Symbol** slots;             ///< Array of Symbol* (hash buckets)
    size_t size;                ///< Number of slots
    size_t count;               ///< Number of symbols stored
    struct Symtab* parent;      ///< Parent scope (NULL for global)
    Arena** arena;              ///< Arena for allocations
} Symtab;

/* --------------- Core API ------------ */

/**
 * @brief Create a new symbol table
 * @param parent Parent scope, or NULL for global scope
 * @param arena Arena allocator to use
 * @return New symbol table, or NULL on error
 */
Symtab* symtab_new(Symtab* parent, Arena** arena);

/**
 * @brief Define a new symbol or update existing one
 * @param st Symbol table
 * @param name Symbol name
 * @param value Symbol value
 * @param is_const Determine if the Symbol is constant and
 *                  promote the existent symbol if not already constant
 * @return The symbol, or NULL on error
 */
Symbol* symtab_define(Symtab* st, const char* name, Cons* value, bool is_const);

/**
 * @brief Lookup a symbol in this scope and parent scopes
 * @param st Symbol table
 * @param name Symbol name to lookup
 * @return The symbol if found, NULL otherwise
 */
Symbol* symtab_lookup(const Symtab* st, const char* name);

/**
 * @brief Lookup a symbol only in current scope (not parents)
 * @param st Symbol table
 * @param name Symbol name to lookup
 * @return The symbol if found, NULL otherwise
 */
Symbol* symtab_lookup_local(const Symtab* st, const char* name);

/**
 * @brief Set value of existing symbol (respects const)
 * @param st Symbol table
 * @param name Symbol name
 * @param value New value
 * @return 0 on success, -1 if not found and -2 if const
 * */
int symtab_set(Symtab* st, const char* name, Cons* value);

/**
 * @brief Create a child scope
 * @param parent Parent symbol table
 * @return New child scope
 */
static inline Symtab* symtab_push_scope(Symtab* parent) {
    return parent ? symtab_new(parent, parent->arena) : NULL;
}

/**
 * @brief Return to parent scope
 * @param st Current symbol table
 * @return Parent scope, or NULL if global
 */
static inline Symtab* symtab_pop_scope(Symtab* st) {
    return st ? st->parent : NULL;
}

/* --------------- Utility Functions ------------ */

/**
 * @brief Get number of symbols in table (not including parent scopes)
 */
static inline size_t symtab_count(const Symtab* st) {
    return st ? st->count : 0;
}

/**
 * @brief Check if symbol table is global scope
 */
static inline bool symtab_is_global(const Symtab* st) {
    return st && st->parent == NULL;
}

/**
 * @brief Hash function for symbol names (FNV-1a)
 */
uint32_t symtab_hash(const char* str);

#endif
