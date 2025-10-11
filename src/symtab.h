/**
 * @file symtab.h
 * @brief Symbol table for variable and function bindings with lexical scoping
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <aio.h>
#include <unistd.h>

#include "vec.h"
#include "arena.h"

/* ------------------------ TypeTag ----------------------- */

/**
 * @enum SymbolType
 * @brief Classifies the type of value a symbol holds
 */
typedef enum {
    SYM_UNDEFINED,      ///< Symbol declared but not defined
    SYM_INTEGER,        ///< Integer value
    SYM_FLOAT,          ///< Floating-point value
    SYM_STRING,         ///< String value (Always constant)
    SYM_FUNCTION,       ///< User-defined function
    SYM_BUILTIN,        ///< Built-in/primitive function
    SYM_MACRO,          ///< Macro
    SYM_LIST,           ///< List/cons cell
    SYM_BOOLEAN,        ///< Boolean (#t/#f)
    SYM_PROMISE
} SymbolType;

/**
 * @enum SymbolFlags
 * @brief Flags for symbol properties
 */
typedef enum {
    SYM_FLAG_NONE      = 0,
    SYM_FLAG_CONST     = 1 << 0,  ///< Immutable (define)
    SYM_FLAG_MUTABLE   = 1 << 1,  ///< Mutable (let, set!)
    SYM_FLAG_GLOBAL    = 1 << 2,  ///< Global scope
    SYM_FLAG_EXPORTED  = 1 << 3,  ///< Exported from module
    SYM_FLAG_TEMPORANY = 1 << 4,  ///< Only present in the eval stack,
                                  ///  not dynamic like the others
    SYM_FLAG_PROMISE = 1 << 5,   ///< Only for temporized object (IO future/promise)
} SymbolFlags;


/* ----------------------- Symbol Struct ---------------------- */

typedef struct Symbol Symbol;


/* --------------------- Standard Values ------------------- */

/**
 * @struct lambda
 * @brief This is a function in form of two Vectors of Tokens.
 */
typedef struct lambda {
    Vec* params;
    Vec* body;
} lambda;

/**
 * @union SymbolValue
 * @brief Tagged union for different value types
 */
typedef struct promise promise; ///< For future/promise Async. @see promise struct below
typedef union {
    long long int_val;
    double float_val;
    char* str_val;
    void* ptr_val;          ///< Generic pointer (for Built-in functions, etc.)
    promise* promise;       ///< @see Promise struct
    lambda* func_val;
    Vec* list_val;
    bool bool_val;
} SymbolValue;

/**
 * @struct Symbol
 * @brief Represents a symbol binding in the environment
 */
typedef struct Symbol {
    char* name;             ///< Symbol name (interned string)
    size_t name_len;        ///< Length of name (for fast comparison)
    uint32_t name_hash;     ///< Pre-computed hash for lookup

    SymbolType type;        ///< Type of value
    SymbolFlags flags;      ///< Symbol properties
    SymbolValue value;      ///< Actual value

    struct Symbol* next;    ///< For hash collision chaining
} Symbol;


/* ----------------------  SymTab  ------------------------- */

/**
 * @struct SymTab
 * @brief Hash table for symbol storage with lexical scoping
 */
typedef struct SymTab {
    size_t bucket_count;    ///< Number of buckets (power of 2)
    size_t symbol_count;    ///< Total symbols in this scope

    struct SymTab* parent;  ///< Parent scope (for lexical scoping)
    size_t depth;           ///< Scope depth (0 = global)

    Arena* arena;           ///< An arena to manage all non-banal values
    Symbol* buckets[];      ///< Hash table buckets
} SymTab;


/* ----------------- Initialization & Cleanup ----------------- */

/**
 * @brief Create a new symbol table
 * @param[in] initial_capacity Initial number of buckets (rounded to power of 2)
 * @param[in] parent Parent scope (NULL for global scope)
 * @param[in] depth This is the depth of the symtab in the hierarchy
 * @return Pointer to new symbol table, or NULL on failure
 */
SymTab*
symtab_new(size_t initial_capacity, SymTab* parent, Arena* arena);

/**
 * @brief Destroy symbol table and free all symbols
 * @param[in/out] table A reference Symbol table to destroy
 * @param[in] recursive If true, also destroy parent scopes
 * @note This function for safety reasons will null the table 
 *       to prevent use-after-free.
 */
void symtab_destroy(SymTab** table, bool recursive);


/* ----------------- Symbol Creation ----------------- */

/**
 * @brief Add or update a symbol in the current scope
 * @param[in] table Symbol table
 * @param[in] name Symbol name
 * @param[in] type Symbol type
 * @param[in] value Symbol value
 * @param[in] flags Symbol flags
 * @param[in] val_size This is a memory region after the Symbol
 *                 to allocate their value if its a pointer 
 *                 of user-defined (usually) object.
 * @return 0 on success, negative on error
 * @retval -1 Table is NULL
 * @retval -2 Name is NULL/empty
 * @retval -3 Symbol is const and already exists
 */
int symtab_define(SymTab* table, const char* name, size_t name_len,
                  SymbolType type, SymbolValue value,
                  SymbolFlags flags);

/**
 * @brief Modifies the value of an existing symbol, searching from the current
 *        scope upwards to the global one.
 *
 * @param table      The starting scope for the search.
 * @param name       The name of the symbol to modify.
 * @param name_len   The length of the name.
 * @param type       The new type of the symbol.
 * @param value      The new value for the symbol.
 *
 * @return 0 on success, a negative value on error.
 * @retval 0  Success: the symbol has been updated.
 * @retval -1: invalid input parameters (table or name are NULL).
 * @retval -2: symbol not found in any accessible scope.
 * @retval -3: the symbol is a constant and cannot be modified.
 * @retval -4: failed to allocate memory for the new value.
 */
int symtab_set(SymTab* table, const char* name, size_t name_len,
               SymbolType type, SymbolValue value);


/* ----------------- Symbol Lookup ----------------- */

/**
 * @brief Looks up a symbol by name, searching from the current scope
 *        upwards to the global scope.
 *
 * @param table     The starting symbol table (scope) for the search.
 * @param name      The name of the symbol to find.
 * @param name_len  The length of the symbol's name.
 * @return          A pointer to the found Symbol, or NULL if the symbol
 *                  is not defined in any accessible scope.
 */
Symbol* symtab_lookup(const SymTab* table, const char* name, const size_t name_len);

/**
 * @brief Look up a symbol only in current scope (no parent search)
 * @param table Symbol table
 * @param name Symbol name
 * @return Pointer to symbol, or NULL if not found
 */
Symbol* symtab_lookup_local(const SymTab* table, const char* name, const size_t name_len);


/* ----------------- Symbol Removal ----------------- */

/**
 * @brief Remove a symbol from current scope only
 * @param table Symbol table
 * @param name Symbol name
 * @return 0 on success, negative on error
 * @retval -1 Table is NULL
 * @retval -2 Symbol not found
 * @retval -3 Symbol is const
 */
int symtab_remove(SymTab* table, const char* name, const size_t name_len);


/* ------------------------- Scope Management ------------------------- */

/**
 * @brief Create a new nested scope
 * @param parent Parent scope
 * @return New child scope, or NULL on failure
 */
SymTab* symtab_push_scope(SymTab* parent);

/**
 * @brief Destroy current scope and return parent
 * @param table Current scope
 * @return Parent scope, or NULL if table was global
 */
SymTab* symtab_pop_scope(SymTab* table);


/* --------------------------- Utilities ----------------------------- */

/**
 * @brief Get number of symbols in current scope
 * @param table Symbol table
 * @return Number of symbols, or 0 if NULL
 */
size_t symtab_size(const SymTab* table);

/**
 * @brief Print all symbols in current scope (for debugging)
 * @param table Symbol table
 */
void symtab_dump(const SymTab* table);

/**
 * @brief Check if symbol exists in current or parent scopes
 * @param table Symbol table
 * @param name Symbol name
 * @return true if symbol exists, false otherwise
 */
bool symtab_exists(const SymTab* table, const char* name, const size_t name_len);

/* --------------- FNV-1a Hash Function Implementation -------------------- */

#define FNV_PRIME_32 ((uint32_t)16777619)
#define FNV_OFFSET_BASIS_32 ((uint32_t)2166136261)

/**
 * @brief Compute the 32-bit FNV-1a hash for a string.
 * @param str String to hash.
 * @param len Length of the string.
 * @return 32-bit hash value.
 */
static inline uint32_t symtab_hash(const char* str, size_t len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)str[i];
        hash *= FNV_PRIME_32;
    }
    return hash;
}


/* ---------------------- Temporal API ------------------ */

/**
 * @struct Promise
 * @brief Lightweight I/O tracker placeholder
 * 
 * This is NOT the final value. It's just a marker that says
 * "I/O operation in progress, check back later".
 */
#include <aio.h>
typedef struct promise {
    struct aiocb cb;           ///< POSIX AIO control block

    char* buffer;              ///< Data buffer (owned by promise)
    size_t buffer_size;        ///< Buffer capacity

    Symbol* target_symbol;     ///< Symbol to update when I/O completes
    SymbolType result_type;    ///< Expected result type (SYM_STRING, SYM_LIST, etc.)

    struct promise* next;      ///< Next promise in tracker queue
} promise;


/* ---------------- Global promise tracker -------------- */

/**
 * @struct PromiseTracker
 * @brief Global tracker for all pending I/O operations
 */
typedef struct PromiseTracker {
    promise* pending_head;     ///< Lista di promise pendenti (linked list)
    size_t pending_count;      ///< Numero di promise attive

    struct aiocb** aiocb_list; ///< Array per aio_suspend() call
    size_t capacity;           ///< Capacità dell'array aiocb_list
} PromiseTracker;

/**
 * @brief Global singleton instance of the promise tracker
 * 
 * This is a file-scope static variable, meaning it's only visible
 * within promise.c. External code accesses it through the public API
 * (promise_tracker_init, promise_tracker_poll, etc.)
 */
static PromiseTracker* g_tracker = NULL;

/* -------------------- Initialization -------------------- */

/**
 * @brief Initializes the global promise tracker singleton.
 *
 * This function sets up the global `g_tracker` instance, which is required for
 * managing all asynchronous I/O operations (promises). It allocates memory for
 * the tracker structure and its internal list from the provided symbol table's
 * arena.
 *
 * This function is idempotent; if the tracker has already been initialized,
 * it will do nothing.
 *
 * @param[in] st A pointer to a symbol table, used to access its memory arena
 *               for allocations. This should typically be the global symbol table.
 *
 * @note This function must be called once at application startup before any
 *       promise-based operations are created or registered.
 * @note The line `g_tracker->aiocb_list = arena_alloc(&st->arena, sizeof(struct aiocb*));`
 *       allocates space for only a single pointer. It should likely be
 *       `g_tracker->capacity * sizeof(struct aiocb*)` to match the intended capacity.
 */
void promise_tracker_init(SymTab* st) {
    if (g_tracker) return;

    g_tracker = arena_alloc(&st->arena, sizeof(PromiseTracker));
    g_tracker->capacity = 64;
    g_tracker->aiocb_list = arena_alloc(&st->arena, sizeof(struct aiocb*));
    g_tracker->pending_head = NULL;
}

/**
 * @brief Removes a promise from the global tracker and cleans up its resources.
 *
 * This function performs two main tasks:
 * 1. It finds and removes the specified promise `p` from the `g_tracker`'s
 *    linked list of pending operations.
 * 2. It cleans up all resources associated with the promise, including attempting
 *    to cancel the underlying AIO operation if it is still in progress,
 *    closing the file descriptor, and freeing the promise's data buffer and the
 *    promise struct itself.
 *
 * @param[in] p A pointer to the promise to unregister. The function handles
 *              the case where `p` or the global tracker is NULL.
 *
 * @note This function is typically called when a symbol of type `SYM_PROMISE`
 *       is removed from the symbol table, ensuring that resources for pending
 *       I/O do not leak.
 * @note If the underlying AIO operation cannot be cancelled in a timely manner,
 *       this function will print a warning to stderr and may leak the promise
 *       and its buffer to prevent a crash from a dangling I/O operation.
 */
void promise_unregister(promise* p);

/**
 * @brief Dynamically creates and initializes a new promise structure.
 *
 * This function allocates memory for a new promise and its internal data buffer
 * using the provided memory arena. It then configures the `aiocb` (AIO Control Block)
 * with the specified parameters, preparing it for an asynchronous I/O operation.
 *
 * @param[in,out] arena_ptr   A pointer to the arena pointer. This is used for all
 *                            memory allocations and may be updated if the arena
 *                            needs to grow.
 * @param[in] fd              The file descriptor for the I/O operation.
 * @param[in] buffer_size     The number of bytes to read/write, which also
 *                            determines the size of the allocated data buffer.
 * @param[in] offset          The offset within the file to start the I/O operation.
 * @param[in] target_symbol   A pointer to the symbol that will be updated with the
 *                            result when the I/O operation completes.
 * @param[in] result_type     The `SymbolType` the target symbol's value will have
 *                            after the promise is fulfilled (e.g., SYM_STRING).
 *
 * @return A pointer to the newly created promise on success, or NULL if memory
 *         allocation fails or input parameters are invalid.
 *
 * @note This function only prepares the promise. It does **not** start the I/O
 *       operation (e.g., with `aio_read`) or register the promise with the
 *       global tracker (with `promise_register`). These are separate steps
 *       that must be performed by the caller.
 */

promise* promise_create(Arena** arena_ptr, int fd, size_t buffer_size, off_t offset,
                        Symbol* target_symbol, SymbolType result_type);

/**
 * @brief Registers a new promise with the global tracker.
 *
 * This function adds the given promise to the head of the pending
 * promises linked list, making it eligible for polling by the tracker.
 * It assumes that the global tracker `g_tracker` has already been
 * initialized.
 *
 * @param[in] p A pointer to the promise to be registered. If NULL, or if the
 *          global tracker is not initialized, the function does nothing.
 */
static inline void promise_register(promise* p) {
    if (!g_tracker || !p) {
        return;
    }

    p->next = g_tracker->pending_head;

    g_tracker->pending_head = p;

    g_tracker->pending_count++;
}

#endif // SYMTAB_H
