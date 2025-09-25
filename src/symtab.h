/**
 * @file ggc.h
 * @brief Generational Garbage Collector for LISP DSL
 * @details Implements a copying GC for nursery and mark-compact for old generation
 */
#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include "vec.h"


/**
 * @brief A table that stores every unique symbol, ensuring that each symbol name 
 *		  exists only once. This process is known as interning.
 */
typedef struct Vec SymTab;

/* --------------------          Closures and Env         -------------------- */

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
	T_VECTOR,
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

typedef enum func_flags{
	F_IMMUTABLE     = 1u << 0,
	F_PURE          = 1u << 1,
	F_PRIMITIVE     = 1u << 2,
	F_VARIADIC      = 1u << 3,
	F_PINNED        = 1u << 4,
	F_EXTERNAL      = 1u << 5,
	F_CONST_BINDING = 1u << 6
} func_flags;

typedef struct arity_info {
	size_t min_arity;
	size_t max_arity;
} arity_info;

typedef enum gc_colors{
	GC_COLOR_WHITE,
	GC_COLOR_GRAY,
	GC_COLOR_BLACK
} gc_colors;


typedef struct GCInfo{
	size_t gen;
	size_t age;
	gc_colors color;

	size_t obj_size;
} GCInfo;

/* @brief Flags packer via va_list.
 * @param[in] Accepts a variable number of args that will overlap in the flags count
 *			  via a or mask in the functions. This can be considered "like" a utility macro.
*/
static inline uint32_t flags_pack(uint32_t first, ...) {
	va_list ap;
	uint32_t flags = 0;
	va_start(ap, first);
	uint32_t f = first;
	while (f) {
		flags |= f;
		f = (uint32_t)va_arg(ap, unsigned int); /* default promotions */
	}
	va_end(ap);
	return flags;
}

/**
 * @brief An enum defining a set of bit flags.
 *
 * @details These flags describe various properties of an object, function, or
 *          binding. They can be combined using the bitwise OR operator (`|`).
 *
 *          To check if a particular flag is set, use the bitwise AND 
 *          operator (`&`).
 *
 * @code
 * // Example: Create a flags variable for an immutable and primitive object
 * flags my_object_flags = immutable | primitive;
 *
 * // Check if the object is immutable
 * if (my_object_flags & immutable) {
 *     printf("The object is immutable.\n");
 * }
 * @endcode
 */
typedef enum flags {
	/** @brief No special flags are set.
	 *  It's good practice to have a value for the zero/default state.
	 */
	NO_FLAGS      = 0,

	/** @brief The object cannot be modified after its creation. */
	immutable     = 1u << 0,

	/** @brief Indicates that a function is pure, i.e., it has no side effects
	 *         and its output depends solely on its inputs.
	 */
	pure          = 1u << 1,

	/** @brief The object represents a primitive data type or function(e.g., integer, boolean). */
	primitive     = 1u << 2,

	/** @brief The function accepts a variable number of arguments (like printf). */
	variadic      = 1u << 3,

	/** @brief The object is "pinned" in memory and cannot be moved or reallocated
	 *         by a memory manager (e.g., a garbage collector).
	 */
	pinned        = 1u << 4,

	/** @brief The object or function is defined in another compilation unit
	 *         or external library.
	 */
	external      = 1u << 5,

	/** @brief The variable's binding is constant and cannot be reassigned. 
	 *         Similar to the `const` keyword in C.
	 */
	const_binding = 1u << 6
} flags;

/** @brief Check if a flag is identical to the bit mask. */
static inline int  tst_flag(uint32_t flags, uint32_t bit) { return (flags & bit) != 0; }
/** @brief Add a specified number of flags via the bit list. */
static inline void add_flag(uint32_t* flags, uint32_t bit){ *flags |= bit; }
/** @brief Removes the flag set to 1 in bit in the flags param with a mask. */
static inline void clr_flag(uint32_t* flags, uint32_t bit){ *flags &= ~bit; }

/* -------------------- Symbol, SymTab, and Helpers -------------------- */

/**
 * @brief Represents a symbol (identifier) in the language.
 * @details A symbol is a unique object that represents a name. It holds
 *          all the metadata associated with that name, such as its type,
 *          value, arity (if it's a function), flags, and GC information.
 */
typedef struct Symbol {
	/* Core identification data */
	const char* name;
	/* @brief The name of the symbol (as a string). */

	size_t len;
	/* @brief The length of the name. */ 

	uint32_t hash;
	/* @brief Pre-calculated hash of the name for fast lookups. */


	/* Semantic metadata */ 
	TypeTag type;
	/* @brief The high-level type of the symbol. @see TypeTag */ 

	EvalKind eval;
	/* @brief How the evaluator should handle this symbol. @see EvalKind */

	uint32_t flags;     
	/* @brief A bitmask of additional properties. @see flags */

	arity_info arity;     
	/* @brief Packed min/max arity. */

	void* value_ptr; 
	/* @brief Pointers to data and code. @see ConsData and other 
	 * structs below to acknowledge the full list of native datatypes.*/

	EnvFrame* env_ptr;
	/* @brief Pointer to the closure environment (for functions). */

	/* Runtime metadata */
	GCInfo gcinfo;    
	/* @brief Information for the Garbage Collector. */
} Symbol;


/* Symbol types implementation, this goes in value_ptr of symbol. @see Symbol.value_ptr. 
 * T_NIL does not have a struct value, its simply and always NULL. If a Symbol of type NIL 
 * have a non-null pointer there is surely a bug.
 **/
typedef struct ConsData {
    Symbol* car;
    Symbol* cdr;
} ConsData;

typedef struct SymData {
	Symbol* sym;
} SymData;

typedef struct IntData {
    int value;
} IntData;

typedef struct StringData {
    char* str;
    size_t length;
    size_t capacity;
} StringData;

typedef struct FloatData {
    double value;
} FloatData;

typedef struct VectorData {
	Vec* vec;    ///> This vec should be managed with the vec.h functions
} VectorData;

typedef struct BoolData {
    bool value;
} BoolData;


typedef Symbol* (*CFunctionFn)(Symbol* args, EnvFrame* env);
typedef struct FunctionData {
    Symbol* params;        ///> Parameter list (NULL for some functions, note that zero-arg 
						   ///  is not the same as having nil as an arg. Nil is still an empty symbol, valueless)
    
    bool is_c_func;        ///> true = C function, false = LISP function
    
    union {
        struct {
            Symbol* body;           ///> Corpo della funzione LISP
            EnvFrame* closure_env;  ///> Environment catturato
        } lisp;
        
        struct {
            CFunctionFn fn;         ///> Funzione C normale  
            const char* c_name;     ///> Name for debug
        } c;
    } func;
    
    uint8_t min_args;
    uint8_t max_args;       ///> 255 = variadic
} FunctionData;


typedef struct MacroData {
    Symbol* params;    ///> Parameter list
    Symbol* body;      ///> Macro expansion template
} MacroData;

typedef Symbol* (*SpecialFormFn)(Symbol* args, EnvFrame* env);

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
 * @param type Object type for GC tracing
 * @param size Size of object to allocate
 * @return Pointer to allocated and interned symbol
 */
Symbol* symtab_intern(SymTab* st, size_t size);

/**
 * @brief Removes symbol from root set (becomes eligible for collection).
 * @param st Symbol table context  
 * @param symbol Symbol to remove
 */
void symtab_remove(SymTab* st, Symbol* symbol);

/**
 * @brief Binds name to symbol for lookup.
 * @param st Symbol table context
 * @param name Variable name
 * @param symbol Symbol to bind
 */
void symtab_bind(SymTab* st, char* name, Symbol* symbol);

/**
 * @brief Looks up symbol by name.
 * @param st Symbol table context
 * @param name Variable name to lookup
 * @return Symbol if found, NULL otherwise
 */
Symbol* symtab_lookup(SymTab* st, char* name);

/**
 * @brief Removes name binding (symbol remains alive if referenced elsewhere).
 * @param st Symbol table context
 * @param name Variable name to unbind
 */
void symtab_unbind(SymTab* st, char* name);


#endif
