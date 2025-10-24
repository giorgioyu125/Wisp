/**
 * @file parser.h
 * @brief Parser types and API for building Cons lists from tokens.
 */
#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#include "arena.h"
#include "vec.h"

/* ------------------------ Node Types ------------------------ */

typedef enum NodeType{
    NODE_ATOM_INT,          ///< car[] holds long long
    NODE_ATOM_FLOAT,        ///< car[] holds double
    NODE_ATOM_SYM,          ///< car[] holds char* (symbol)
    NODE_ATOM_STR,          ///< car[] holds char* (string)
    NODE_ATOM_UNINTERNED,   ///< car[] holds char* (uninterned symbol #:foo)
    NODE_LIST,              ///< car[] holds ConsList*
    NODE_CLOSING_SEPARATOR, ///< Parser artifact
    NODE_OPENING_SEPARATOR, ///< Parser artifact
    NODE_NIL                ///< car[] holds NULL
} NodeType;

/* ------------------------ Cons Cells ------------------------ */

typedef struct Cons {
    NodeType type;
    struct Cons* cdr;    ///< Next cons cell
    size_t size;         ///< Payload size in bytes
    _Alignas(max_align_t) unsigned char car[];
} Cons;

typedef struct {
    Cons*  head;        ///< Where the program starts
    Cons*  tail;        ///< Where it ends
    size_t length;      ///< Program lenght in cons cell
} ConsList;

/* --------------------- Public API --------------------------- */

/**
 * Create an atom/list cons cell with payload copied into car[].
 */
Cons* make_atom(const void* data, size_t size, NodeType type, Arena** arena);

static inline Cons* make_shallow_atom(NodeType type, const char* p, size_t n, Arena** arena) {
    size_t sz = n + 1;
    Cons* c = (Cons*)arena_alloc(arena, sizeof(Cons) + sz);
    if (!c) return NULL;
    c->type = type;
    c->cdr  = NULL;
    c->size = sz;
    memcpy(c->car, p, n);
    ((char*)c->car)[n] = '\0';
    return c;
}

/**
 * Parse a parenthesized list starting at an opening separator.
 * Advances *cursor to the node after the matching ')'.
 * Returns a NODE_LIST cons whose car[] holds a ConsList*.
 */
Cons* parse_list(Cons** cursor, Arena** arena);

/**
 * @brief Linearize a Vec<Token> into a flat Cons chain and parse one S-expression.
 * @retval ConsList* containing the parsed top-level form.
 */
ConsList* parse_sexpr(Vec* tokens, Arena** arena);

/**
 * @brief Parses a vector of tokens containing multiple top-level S-expressions.
 *
 * @note This function identifies all separate top-level expressions in the token stream,
 *       iteratively parses each one, and bundles the resulting ASTs into a single
 *       ConsList representing the entire program.
 */
ConsList* parse_program(Vec* tokens, Arena** arena);

/* -------------------- Inline Utilities ---------------------- */

static inline void cons_list_init(ConsList* l) {
    l->head = NULL;
    l->length = 0;
}

static inline void cons_list_push_back(ConsList* l, Cons* n) {
    if (!l->head) {
        l->head = l->tail = n;
        n->cdr = NULL;
        l->length = 1;
    } else {
        l->tail->cdr = n;
        l->tail = n;
        n->cdr = NULL;
        l->length++;
    }
}

static inline Cons* cons_clone_shallow(const Cons* src, Arena** arena) {
    Cons* c = (Cons*)arena_alloc(arena, sizeof(Cons) + src->size);
    if (!c) return NULL;
    c->type = src->type;
    c->cdr  = NULL;
    c->size = src->size;
    if (src->size) memcpy(c->car, src->car, src->size);
    return c;
}

static inline Cons* wrap_list(ConsList* list, Arena** arena) {
    return make_atom(&list, sizeof(ConsList*), NODE_LIST, arena);
}

static inline bool is_sublist(const Cons* c) {
    return c && c->type == NODE_LIST;
}

/* ------------ Debugging ----------- */


/**
 * @brief Prints an entire program (a list of top-level S-expressions).
 *
 * This function iterates through the main program list and prints each
 * top-level expression on a new line.
 *
 * @param program A pointer to the ConsList representing the program.
 */
void print_program(ConsList* program);

#endif /* PARSER_H */
