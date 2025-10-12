/*
* @file parser.h
* @brief A expression annotator based parser to determine the id of the tokens and turn them 
*        into a pair of virtual stacks of Operation/Values
*/

#ifndef PARSER_H
#define PARSER_H

#include "vec.h"
#include <stddef.h>


/* ----------------- S-Expr --------------- */

/**
 * @brief S-Expression representation as a slice of the token vector
 */
typedef struct SExpr {
    size_t id;              ///< The s_exprid
    size_t start_idx;       ///< First token index with this id
    size_t end_idx;         ///< Last token index with this id
    Vec* tokens;            ///< Reference to original slice of tokens vector
} SExpr;

/**
 * @brief Program flux - ordered sequence of s-expressions
 */
typedef struct ProgramFlux {
    Vec* sexprs;            ///< Vec<SExpr*>
    Vec* tokens;            ///< Original flat token vector
    size_t max_depth;       ///< Maximum nesting depth
} ProgramFlux;

/* ----------------- Annotator --------------- */

/*
 * @brief This functions updates the s_exprid field of every 
 *        accordingly and removes parenthesis.
 * @param[out] Tokens is the return value of the lexer
 *        and the out param of the annotator.
 * @retval This signal if an error occured while parsing the Tokens.
 */
int annotate_tokens(Vec * Tokens);

/* -------------- Parser ------------- */

/**
 * @brief Parses an annotated token vector into a structured program flux.
 *
 * ## Overview
 *
 * This function is the second phase of the Wisp compilation pipeline:
 *
 * ```
 * Source Code
 *     ↓
 * [Lexer: lex_tokens()]
 *     ↓
 * Raw Tokens (Vec<Token>)
 *     ↓
 * [Annotator: annotate_tokens()]
 *     ↓
 * Annotated Tokens (with s_exprid)
 *     ↓
 * [Parser: parse()]  ← YOU ARE HERE
 *     ↓
 * ProgramFlux
 *     ↓
 * [Evaluator: eval()]
 *     ↓
 * Runtime Objects
 * ```
 *
 * The `parse` function takes the flat vector of tokens that have been annotated
 * with `s_exprid` values (which identify which S-expression each token belongs to)
 * and transforms it into a `ProgramFlux`: a structured representation optimized
 * for evaluation.
 *
 * ## What This Function Does
 *
 * 1. **Validates S-Expression Structure:**
 *    Ensures that the `s_exprid` annotations form valid, properly nested
 *    S-expressions. This catches structural errors that the annotator might
 *    have missed or that indicate malformed source code.
 *
 * 2. **Identifies Top-Level Expressions:**
 *    Determines which tokens are top-level forms (expressions with `s_exprid == 0`)
 *    versus nested sub-expressions. This is critical for sequential evaluation.
 *
 * 3. **Builds Expression Metadata:**
 *    For each unique `s_exprid`, creates metadata describing:
 *    - The token index range that comprises the expression
 *    - The nesting depth of the expression
 *    - The parent expression (if any)
 *    - The number and positions of child expressions
 *
 * 4. **Optimizes for Evaluation:**
 *    Organizes the data in a way that allows the evaluator to:
 *    - Quickly locate the boundaries of an S-expression
 *    - Iterate over the elements of a list without re-scanning
 *    - Efficiently implement special forms (quote, if, lambda, etc.)
 *
 * 5. **Prepares for Temporal Analysis (Future):**
 *    In later versions with the Causality Schema, this function will also
 *    analyze temporal dependencies and build the initial dependency graph.
 *
 * ## Input Requirements
 *
 * @param[in] annotated_tokens  A pointer to a vector of tokens that has been
 *                              processed by both `lex_tokens()` and `annotate_tokens()`.
 *
 * **The input vector MUST satisfy these preconditions:**
 *
 * - **Non-NULL and Valid:** The pointer must not be NULL, and the vector must
 *   be properly initialized.
 *
 * - **Already Annotated:** Every token in the vector must have a valid `s_exprid`
 *   value assigned by `annotate_tokens()`. Tokens that were `TOKEN_LPAREN` or
 *   `TOKEN_RPAREN` should now be `TOKEN_IGNORE`.
 *
 * - **Balanced Expressions:** The `s_exprid` values must represent a balanced
 *   tree of S-expressions. If `annotate_tokens()` returned 0 (success), this
 *   is guaranteed.
 *
 * - **Ownership:** The caller retains ownership of the token vector. `parse()`
 *   will NOT free it, but the returned `ProgramFlux` will hold a pointer to it.
 *   The token vector must remain valid for the lifetime of the `ProgramFlux`.
 *
 * ## Return Value
 *
 * @return A pointer to a newly allocated `ProgramFlux` structure on success,
 *         or `NULL` on failure.
 *
 * **Success:**
 * - A valid `ProgramFlux*` is returned.
 * - The caller is responsible for eventually freeing this structure using
 *   `program_flux_free()`.
 * - The `ProgramFlux` holds a reference to the original `annotated_tokens` vector,
 *   which must not be freed or modified while the `ProgramFlux` is in use.
 *
 * **Failure (returns NULL):**
 * - **Invalid Input:** `annotated_tokens` is NULL or the vector is invalid.
 * - **Malformed Tokens:** The token vector contains inconsistent `s_exprid` values
 *   that don't form a valid expression tree (e.g., orphaned IDs, cycles).
 * - **Memory Allocation Error:** The system could not allocate memory for the
 *   `ProgramFlux` or its internal structures.
 * - **Empty Program:** The token vector contains no evaluable expressions
 *   (only whitespace and comments). This may or may not be an error depending
 *   on your use case.
 *
 * ## Error Handling
 *
 * On error, this function:
 * 1. Returns `NULL`.
 * 2. Does NOT modify the input `annotated_tokens` vector.
 * 3. Cleans up any partially allocated internal structures.
 * 4. (Optionally) Logs an error message to `stderr` describing the failure.
 *
 * The caller should check for `NULL` and handle the error appropriately:
 *
 * ```c
 * ProgramFlux* flux = parse(tokens);
 * if (!flux) {
 *     fprintf(stderr, "parse: failed to parse token stream\n");
 *     vec_free(&tokens);
 *     return -1;
 * }
 * ```
 *
 * ## Design Rationale: Why Not an AST?
 *
 * Traditional compilers build an Abstract Syntax Tree (AST), where the code is
 * represented as a hierarchical tree of nodes in memory. Wisp takes a different
 * approach for several reasons:
 *
 * ### 1. Memory Efficiency
 * An AST duplicates information already present in the token stream. By using
 * `s_exprid` annotations on the flat token vector, we avoid allocating a large
 * tree structure. The "virtual tree" is implicit in the IDs.
 *
 * ### 2. Cache Locality
 * A flat vector has excellent cache locality. Traversing an AST means following
 * pointers all over the heap, leading to cache misses. The `ProgramFlux` keeps
 * data contiguous, making evaluation faster.
 *
 * ### 3. Simplicity
 * There's no need to write recursive tree-building code, manage parent/child
 * pointers, or implement tree traversal utilities. The parser is simpler and
 * less error-prone.
 *
 * ### 4. Homoiconicity (Code-as-Data)
 * Lisp's power comes from treating code as data. A flat vector of tokens is
 * much closer to the original list structure of the source code than a C struct
 * tree. This makes implementing macros and code manipulation easier.
 *
 * ### 5. Temporal Alignment
 * Wisp's philosophy is that "something is only defined by its behavior in time."
 * A program isn't a static tree; it's a *flow* of expressions. The `ProgramFlux`
 * name and design reflect this: it's a stream of computational events, not a
 * frozen structure.
 *
 * ## Usage Example
 *
 * Here's a complete example of using `parse()` in the compilation pipeline:
 *
 * ```c
 * #include "lexer.h"
 * #include "parser.h"
 * #include "eval.h"
 * #include "readfile.h"
 *
 * int main(int argc, char** argv) {
 *     // 1. Read source file
 *     FileBuffer* file = read_file(argv[1]);
 *     if (!file) {
 *         fprintf(stderr, "Failed to read file\n");
 *         return -1;
 *     }
 *
 *     // 2. Lex into tokens
 *     Vec* tokens = lex_tokens(file->data, file->size);
 *     if (!tokens) {
 *         fprintf(stderr, "Lexing failed\n");
 *         filebuffer_free(file);
 *         return -1;
 *     }
 *
 *     // 3. Annotate tokens with s_exprid
 *     int err = annotate_tokens(tokens);
 *     if (err != 0) {
 *         fprintf(stderr, "Annotation failed with error %d\n", err);
 *         vec_free(&tokens);
 *         filebuffer_free(file);
 *         return -1;
 *     }
 *
 *     // 4. Parse into ProgramFlux
 *     ProgramFlux* program = parse(tokens);
 *     if (!program) {
 *         fprintf(stderr, "Parsing failed\n");
 *         vec_free(&tokens);
 *         filebuffer_free(file);
 *         return -1;
 *     }
 *
 *     // 5. Evaluate the program
 *     Object* result = eval(program, global_env);
 *
 *     // 6. Cleanup (note: tokens must outlive program)
 *     program_flux_free(program);
 *     vec_free(&tokens);
 *     filebuffer_free(file);
 *
 *     return 0;
 * }
 * ```
 *
 * ## Memory Management
 *
 * ### Ownership Model:
 * - **Input:** The caller owns `annotated_tokens` and must keep it alive.
 * - **Output:** The caller owns the returned `ProgramFlux*` and must free it.
 *
 * ### Lifetime Dependencies:
 * ```
 * annotated_tokens (Vec*)
 *     ↓ referenced by
 * ProgramFlux
 *     ↓ used by
 * eval() and runtime
 * ```
 *
 * **Critical:** The `annotated_tokens` vector MUST remain valid and unmodified
 * for the entire lifetime of the `ProgramFlux`. Do not free or modify tokens
 * until after calling `program_flux_free()`.
 *
 * ### Cleanup Function:
 * ```c
 * void program_flux_free(ProgramFlux* flux);
 * ```
 * This function:
 * - Frees all internal metadata structures
 * - Does NOT free the original token vector (caller's responsibility)
 * - Sets internal pointers to NULL for safety
 * - Is safe to call with NULL (no-op)
 *
 * ## Thread Safety
 *
 * This function is **NOT thread-safe**. Do not call `parse()` on the same
 * `annotated_tokens` vector from multiple threads simultaneously.
 *
 * ## Performance Characteristics
 *
 * - **Time Complexity:** O(n), where n is the number of tokens.
 *   The function makes a single linear pass through the token vector.
 *
 * - **Space Complexity:** O(e), where e is the number of unique S-expressions.
 *   The `ProgramFlux` stores metadata for each unique `s_exprid`.
 *
 * - **Typical Performance:** For a 10,000-token program, parsing takes on the
 *   order of microseconds on modern hardware.
 *
 * ## Future Extensions
 *
 * In future versions (post-v0.1), `parse()` may also:
 *
 * 1. **Causality Schema Validation:**
 *    When a `define-causality-schema` block is present, `parse()` will extract
 *    the precedence relations and invariants, storing them in the `ProgramFlux`
 *    for use by the evaluator.
 *
 * 2. **Temporal Dependency Analysis:**
 *    Static analysis to build the initial dependency graph for behaviors and
 *    streams, identifying which parts of the program are temporally connected.
 *
 * 3. **Optimization Passes:**
 *    Simple optimizations like constant folding, dead code elimination for
 *    `quote`d but unused data, and inlining of trivial lambdas.
 *
 * 4. **Metadata Extraction:**
 *    Extracting and storing docstrings, type hints, or other metadata attached
 *    to definitions.
 *
 * @see annotate_tokens() - The function that must be called before parse()
 * @see eval() - The function that consumes the ProgramFlux
 * @see program_flux_free() - Cleanup function for the returned structure
 *
 * @note This function is part of the core compilation pipeline and is called
 *       exactly once per program or REPL input.
 */
ProgramFlux* parse(Vec* annotated_tokens);


/**
 * @brief Frees all memory associated with a ProgramFlux.
 *
 * @param[in] flux  The ProgramFlux to free. Can be NULL (no-op).
 *
 * @note This does NOT free the original token vector that was passed to `parse()`.
 *       The caller is still responsible for that.
 *
 * @warning After calling this function, the `flux` pointer is invalid and must
 *          not be used. It is recommended to set it to NULL:
 *          ```c
 *          program_flux_free(flux);
 *          flux = NULL;
 *          ```
 */
void program_flux_free(ProgramFlux* flux);

/* --------------------------- Debugging ------------------------- */

/**
 * @brief Debug utility: prints the structure of a ProgramFlux to stderr.
 *
 * This function is intended for development and debugging. It prints:
 * - The number of top-level expressions
 * - The number of unique S-expressions
 * - A hierarchical view of the expression structure
 *
 * @param[in] flux  The ProgramFlux to dump. If NULL, prints an error message.
 *
 * Example output:
 * ```
 * ============== PROGRAM FLUX DUMP ==============
 * Top-level expressions: 3
 * Unique s-expressions: 7
 *
 * [0] Top-level (tokens 0-5)
 *   [1] List (tokens 1-4)
 *     [2] Symbol: define (token 1)
 *     [3] Symbol: x (token 2)
 *     [4] Integer: 42 (token 3)
 * [0] Top-level (tokens 6-10)
 *   [5] List (tokens 7-9)
 *     [6] Symbol: + (token 7)
 *     [7] Symbol: x (token 8)
 *     [4] Integer: 1 (token 9)
 * ===============================================
 * ```
 */
void program_flux_dump(const ProgramFlux* flux);



#endif
