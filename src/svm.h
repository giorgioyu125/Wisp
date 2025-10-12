/*
 * @file svm.h
 * @brief Virtual machine interface for the interpreter.
 */
#ifndef SVM_H
#define SVM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "parser.h"
#include "symtab.h"
#include "lexer.h"
#include "arena.h"

/* ---------------------------- VM Types -------------------------- */

typedef struct {
    SymbolType  type;
    SymbolValue val;
} Value;

typedef struct {
    Arena* eval_cache; ///< This contains the result of the expressions
    SExpr* pc;         ///< This is the program counter and the implicit stack
} VM;

enum {
    SVM_OK = 0,
    SVM_ERR_ARG   = -1,
    SVM_ERR_TYPE  = -2,
    SVM_ERR_UNBOUND = -3,
    SVM_ERR_OOM   = -4,
    SVM_ERR_EVAL  = -5,
};

extern PromiseTracker* g_tracker;
extern SymTab* g_symtab;

/* --------------------------- Built-in API ----------------------- */

typedef int (*BuiltinFn)(VM* vm,
                         SymTab* env,
                         const SExpr* call,
                         Value* out);

/* -------------------------- VM lifecycle ----------------------- */


static inline SExpr* select_first_expr(const ProgramFlux* prog) { return vec_at(prog->sexprs, 0); }

VM*  svm_create(const ProgramFlux* prog);
void svm_destroy(VM* vm);

static inline bool svm_ensure_cache(VM* vm, size_t need_capacity) { return vm->eval_cache->capacity > need_capacity ? true : false; }
static inline void svm_clear_cache(VM* vm) { arena_reset(vm->eval_cache); }

/* --------------------------- Evaluation ------------------------- */

int svm_eval_expr(VM* vm, const ProgramFlux* prog, const SExpr* expr, Value* out);
int svm_eval_atom(VM* vm, SymTab* env, const Token* tok, Value* out);
int svm_eval_list(VM* vm, SymTab* env, const ProgramFlux* prog, const SExpr* list, Value* out);

typedef enum { ELEM_ATOM, ELEM_SUBLIST } ElemKind;
typedef struct {
    ElemKind kind;
    Token    atom;     /* valid if kind == ELEM_ATOM */
    size_t   child_id; /* valid if kind == ELEM_SUBLIST */
} ListElem;

/* ------------------------- Builtins ----------------------- */
/* Call-site aware builtins; consistent ABI */
int bltn_add   (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_sub   (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_mul   (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_div   (VM* vm, SymTab* env, const SExpr* call, Value* out);

int bltn_lambda(VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_quote (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_atom  (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_null  (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_cons  (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_car   (VM* vm, SymTab* env, const SExpr* call, Value* out);
int bltn_cdr   (VM* vm, SymTab* env, const SExpr* call, Value* out);

/* --------------------- Register Builtins --------------------- */

/* You can implement this to bind symbols (+, -, *, /, quote, ...) to BuiltinFn
   in g_symtab with type SYM_BUILTIN and value.fn = (BuiltinFn)pointer.
   Note: prefer storing the function pointer in SymbolValue as a BuiltinFn typed field. */
int svm_register_builtins(SymTab* env);

#endif /* SVM_H */
