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
    Arena* eval_cache; ///< This contains the result of the expressions
    Arena* work_cache; ///< Needed by some functions to evaluate efficently
    SExpr* pc;         ///< This is the program counter and the implicit stack
} VM;

typedef struct WorkItem {
    SExpr*  expr;      // expression to evaluate; sentinel if expr->id == SIZE_MAX
    SymTab* env;       // environment for this frame
    lambda* fn;        // for sentinel frames (lambda body driver)
    size_t  body_idx;  // next body form to eval (for sentinel)
    size_t  dest_id;   // where to store result (call-site id) for sentinel
} WorkItem;

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

int svm_eval_atom(VM* vm, SymTab* env, const Token* tok, Value* out);
int svm_step(VM* vm, const ProgramFlux* prog, Vec* stack, size_t root_dest_id, Value* out);
int svm_eval_expr(VM* vm, SymTab* locals, const ProgramFlux* prog, SExpr* expr, Value* out);

int evaluate_program(ProgramFlux* program);

typedef enum { ELEM_ATOM, ELEM_SUBLIST } ElemKind;
typedef struct {
    ElemKind kind;
    Token    atom;     /* valid if kind == ELEM_ATOM */
    size_t   child_id; /* valid if kind == ELEM_SUBLIST */
} ListElem;

/* ------------------------- Builtins ----------------------- */

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

int register_arithmetic_builtins(SymTab* env) {
    if (!env) return -1;
    SymbolValue add_val = { .ptr_val = (void*)bltn_add };
    if (symtab_define(env, "+", 1, SYM_BUILTIN, add_val, SYM_FLAG_CONST) != 0) {
        return -1;
    }
    SymbolValue sub_val = { .ptr_val = (void*)bltn_sub };
    if (symtab_define(env, "-", 1, SYM_BUILTIN, sub_val, SYM_FLAG_CONST) != 0) {
        return -1;
    }
    SymbolValue mul_val = { .ptr_val = (void*)bltn_mul };
    if (symtab_define(env, "*", 1, SYM_BUILTIN, mul_val, SYM_FLAG_CONST) != 0) {
        return -1;
    }
    SymbolValue div_val = { .ptr_val = (void*)bltn_div };
    if (symtab_define(env, "/", 1, SYM_BUILTIN, div_val, SYM_FLAG_CONST) != 0) {
        return -1;
    }
    return 0;
}

#endif /* SVM_H */
