/*
* @file svm.c
* @brief This file contains the implementation of the
*        virtual machine needed to run the interpreter.
*/
#include "svm.h"
#include "arena.h"
#include "parser.h"
#include "symtab.h"

#include "stdlib.h"


VM* svm_create(const ProgramFlux* prog) {
    if (!prog) return NULL;

    VM* vm = malloc(sizeof(VM));
    if (!vm) return NULL;

    size_t cache_size = (prog->max_depth + 1) * sizeof(Value);
    vm->eval_cache = arena_create(cache_size);
    if (!vm->eval_cache) {
        free(vm);
        return NULL;
    }

    vm->pc = select_first_expr(prog);

    return vm;
}

void svm_destroy(VM* vm) {
    if (!vm) return;
    if (vm->eval_cache) arena_free(vm->eval_cache);
    free(vm);
}


int svm_eval_expr(VM* vm, const ProgramFlux* prog, const SExpr* expr, Value* out) {

}

int svm_eval_atom(VM* vm, SymTab* env, const Token* tok, Value* out) {

}

int svm_eval_list(VM* vm, SymTab* env, const ProgramFlux* prog, const SExpr* list, Value* out) {

}
