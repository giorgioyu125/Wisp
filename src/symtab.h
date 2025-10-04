/**
 * @file symtab.h
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "vec.h"
#include "ggc.h"

/**
 * @brief A table that stores every unique symbol, ensuring that each symbol name 
 *		  exists only once. This process is known as interning.
 */
typedef struct Vec SymTab;

/* ----------------------------- Closures and Env ---------------------------- */

/**
 * @brief Environment frame for lexical scoping and closures.
 */
typedef struct EnvFrame {
    SymTab* locals;          ///< Local bindings in this frame
    struct EnvFrame* parent;      ///< Parent environment (lexical chain)
    size_t ref_count;             ///< Number of closures referencing this frame
} EnvFrame;

typedef enum TypeTag{
	T_NIL,
	T_CONS,
	T_SYMBOL,
	T_INT,
	T_STRING,
	T_FLOAT,
	T_BOOL,
	T_FUNCTION,
	T_MACRO,
	T_SPECIAL_FORM
} TypeTag;

typedef enum EvalKind{ 
	EK_DATA, 
	EK_FUNCTION, 
	EK_MACRO, 
	EK_SPECIAL 
} EvalKind;

typedef struct arity_info {
	size_t min_arity;
	size_t max_arity;
} arity_info;


typedef enum flags {
    NO_FLAGS = 0,               ///< No special flags are set.
    immutable = 1 << 0,         ///< The object cannot be modified after creation.
    pure = 1 << 1,              ///< Function has no side effects (output depends only on inputs).
    primitive = 1 << 2,         ///< Represents a primitive type (e.g., integer, boolean).
    variadic = 1 << 3,          ///< Function accepts variable arguments (e.g., printf-style).
    pinned = 1 << 4,            ///< Memory is pinned; cannot be moved by GC/allocator.
    external = 1 << 5,          ///< Defined in another compilation unit/external library.
    const_binding = 1 << 6,     ///< Binding is constant (like C's `const`).
    reachable = 1 << 7,         ///< Object is reachable from the root set (during GC).
    finalizer = 1 << 8,         ///< Object has a finalizer (needs cleanup before collection).
    has_pointers = 1 << 9,      ///< Object contains pointers to other objects (GC must traverse).
    small = 1 << 10,            ///< Object is small (optimization hint for allocator).
    large = 1 << 11,            ///< Object is large (allocated in special region).
    atomic = 1 << 12,           ///< Object supports atomic operations (concurrency).
    deprecated = 1 << 13,       ///< Object is deprecated (may be collected early).
    transient = 1 << 14,        ///< Object is short-lived (likely to die young).
    root = 1 << 15,             ///< Object is a root (global/static/stack variable).
    scanned = 1 << 16,          ///< Object was scanned in the current GC cycle.
    precise = 1 << 17,          ///< Object layout is precisely known (no conservative scanning).
    no_scan = 1 << 18,          ///< Object should not be scanned (e.g., raw data).
    concurrent = 1 << 19,       ///< Object can be accessed concurrently (requires synchronization).
    immortal = 1 << 20,         ///< Object never becomes garbage (e.g., static/global).
    nursery_allocated = 1 << 21,///< Object was allocated in the young generation.
    old_gen_allocated = 1 << 22 ///< Object was promoted to the old generation.
} flags;

// Pack multiple flags into a single bitmask
flags pack_flags(flags first_flag, ...) {
    va_list args;
    flags combined = first_flag;

    va_start(args, first_flag);
    flags next_flag;
    while ((next_flag = va_arg(args, flags)) != 0) {
        combined |= next_flag;
    }
    va_end(args);

    return combined;
}

/** @brief Check if a flag is identical to the bit mask. */
static inline int  tst_flag(uint32_t flags, uint32_t bit) { return (flags & bit) != 0; }
/** @brief Add a specified number of flags via the bit list. */
static inline void add_flag(uint32_t* flags, uint32_t bit){ *flags |= bit; }
/** @brief Removes the flag set to 1 in bit in the flags param with a mask. */
static inline void clr_flag(uint32_t* flags, uint32_t bit){ *flags &= ~bit; }

/* -------------------- Symbol, SymTab, and Helpers -------------------- */


/**
 * @brief A simple dummy hash function using the djb2 algorithm.
 * @details This function computes a 32-bit hash value for a null-terminated string.
 *          It is not cryptographically secure and is intended for basic use, such as
 *          symbol table lookups. The algorithm starts with a seed value and iterates
 *          through each character, combining it into the hash.
 * @param name The null-terminated string to hash.
 * @return A 32-bit unsigned integer hash value.
 */
#define HASH_SEED 5381
uint32_t hash(const char* name) {
    if (name == NULL) {
        return 0;
    }

    uint32_t hash_value = HASH_SEED;
    int c;

    while ((c = *name++)) {
        hash_value = ((hash_value << 5) + hash_value) + c;
    }

    return hash_value;
}


/**
 * @brief Represents a symbol (identifier) in the language.
 * @details A symbol is a unique object that represents a name. It holds
 *          all the metadata associated with that name, such as its type,
 *          value, arity (if it's a function), flags, and GC information.
 */
typedef struct Symbol {
	/* Runtime metadata */
	GCInfo gcinfo;     /* @brief Information for the Garbage Collector. */

	/* Core identification data */
	const char* name;  /* @brief The name of the symbol (as a string). */

	size_t len;        /* @brief The length of the name. */ 

	uint32_t hash;     /* @brief Pre-calculated hash of the name for fast lookups. */

	/* Semantic metadata */ 
	TypeTag type;      /* @brief The high-level type of the symbol. @see TypeTag */ 

	EvalKind eval;     /* @brief How the evaluator should handle this symbol. @see EvalKind */

	uint32_t flags;     /* @brief A bitmask of additional properties. @see flags */
    union value{
        void* value_ptr;
        int32_t int_val;
        double float_val;
        bool bool_val;
    }value;             /* @brief A pointer to the value of the symbol. @see ConsData struct */

	EnvFrame* env_ptr;  /* @brief Pointer to the closure environment (for functions). */
} Symbol;

/* Symbol types implementation, this goes in value_ptr of symbol. @see Symbol.value.value_ptr
 * T_NIL does not have a valid pointer value, its simply and always NULL. If a Symbol of type NIL
 * have a non-null pointer there is surely a bug.
 **/
typedef struct ConsData {
    GCInfo gcinfo;
    Symbol* car;
    Symbol* cdr;
} ConsData;
#define T_CONS_REF_NUM 2

typedef struct SymData {
    GCInfo gcinfo;
	Symbol* sym;
} SymData;
#define T_SYM_REF_NUM 1

typedef struct StringData {
    GCInfo gcinfo;
    char str[];
} StringData;

typedef Symbol* (*CFunctionFn)(Symbol* args, EnvFrame* env);
typedef struct FunctionData {
    GCInfo gcinfo;
    Symbol* params;        ///> Parameter list (NULL for some functions, note that zero-arg 
						   ///  is not the same as having nil as an arg. Nil is still an empty symbol, valueless)

    bool is_c_func;        ///> true = C function, false = LISP function

    union {
        struct {
            Symbol* body;           ///> Body of the LISP function
            EnvFrame* closure_env;  ///> Captured Env
        } lisp;

        struct {
            CFunctionFn fn;         ///> Normal C function
            const char* c_name;     ///> Name for debug
        } c;
    } func;

	arity_info arity;               ///> min/max arity of a function.
} FunctionData;
#define T_FUNC_REF_NUM 3


typedef struct MacroData {
    GCInfo gcinfo;
    Symbol* params;    ///> Parameter list
    Symbol* body;      ///> Macro expansion template
} MacroData;
#define T_MACRO_REF_NUM 2

#define MAX_REF 16

/* ------------ Functions to Extract References from Symbols ------ */

/**
 * @brief Extract references from a cons cell (car and cdr).
 * @param obj Pointer to the cons cell object
 * @param count Output parameter for number of references (always 2)
 * @return Array of pointers to the car and cdr fields
 */
void*** get_reference_from_cons(void* obj, size_t* count);

/**
 * @brief Extract references from a symbol (value, plist, name).
 * @param obj Pointer to the symbol object
 * @param count Output parameter for number of references (always 1)
 * @return Array of pointers to symbol's reference fields
 */
void*** get_reference_from_symbol(void* obj, size_t* count);

/**
 * @brief Extract references from a vector (all elements).
 * @param obj Pointer to the vector object
 * @param count Output parameter for number of elements in vector
 * @return Array of pointers to each vector element
 */
void*** get_reference_from_vector(void* obj, size_t* count);

/**
 * @brief Extract references from a function (body, params, environment).
 * @param obj Pointer to the function object
 * @param count Output parameter for number of references (typically 2)
 * @return Array of pointers to function's reference fields
 */
void*** get_reference_from_function(void* obj, size_t* count);

/**
 * @brief Extract references from a macro (body, transformer, environment).
 * @param obj Pointer to the macro object
 * @param count Output parameter for number of references (typically 2)
 * @return Array of pointers to macro's reference fields
 */
void*** get_reference_from_macro(void* obj, size_t* count);

/* -------------------- Symbol Table Lifetime -------------------- */

/**
 * @brief Creates a new symbol table.
 * @param gc Garbage collector context
 * @return New symbol table instance
 */
SymTab* symtab_create(size_t size);

/**
 * @brief Destroys symbol table and all contained symbols.
 * @param st Symbol table to destroy
 */
void symtab_destroy(SymTab** st);


/* -------------------- Symbol Table API (Main Interface) -------------------- */

/**
 * @brief Allocates and interns a new symbol (automatically becomes root).
 * @param st Symbol table context
 * @param size Size of object to allocate
 * @return Pointer to allocated and interned symbol
 */
Symbol* symtab_intern(Gc* gc, SymTab* st, char* name);

/**
 * @brief Removes symbol from root set (becomes eligible for collection).
 * @param st Symbol table context  
 * @param symbol Symbol to remove
 */
static inline void symtab_remove(SymTab* st, Symbol* symbol) { vec_rem(st, symbol); }

/**
 * @brief Binds name to symbol for lookup.
 * @param st Symbol table context
 * @param name Variable name
 * @param symbol Symbol to bind
 */
static inline void symtab_bind(SymTab* st, char* name, Symbol* symbol) {
    if (!symbol || !name || !st) return;

    symbol->name = name;
}

/**
 * @brief Looks up symbol by name.
 * @param st Symbol table context
 * @param name Variable name to lookup
 * @return Symbol if found, NULL otherwise
 */
Symbol* symtab_lookup(SymTab* st, const char* name);

/**
 * @brief Removes name binding (symbol remains alive if referenced elsewhere).
 * @param st Symbol table context
 * @param symbol Variable pointer to unbind
 */
void symtab_unbind(SymTab* st, Symbol* symbol) { if(symbol || st) symbol->name = NULL;}


#endif
