/*
* @file svm.c
* @brief This file contains the implementation of the
*        virtual machine needed to run the interpreter.
*/
#include "svm.h"
#include "arena.h"
#include "lexer.h"
#include "parser.h"
#include "symtab.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


VM* svm_create(const ProgramFlux* prog) {
    if (!prog) return NULL;

    VM* vm = (VM*)calloc(1, sizeof(VM));
    if (!vm) return NULL;

    size_t slots = prog->max_depth + 1;
    size_t cache_bytes = slots * sizeof(Value);

    vm->eval_cache = arena_create(cache_bytes);
    if (!vm->eval_cache) {
        free(vm);
        return NULL;
    }

    memset(vm->eval_cache->start, 0, cache_bytes);
    Value* cache = (Value*)vm->eval_cache->start;
    for (size_t i = 0; i < slots; ++i) {
        cache[i].type = SYM_UNDEFINED;
        // cache[i].val = (SymbolValue){0};
    }

    size_t cap_exprs    = vec_len(prog->sexprs);
    size_t stack_bytes  = sizeof(Vec) + cap_exprs * sizeof(SExpr*);
    size_t headroom     = 16 * 1024;
    size_t work_bytes   = stack_bytes + headroom;

    vm->work_cache = arena_create(work_bytes > 4096 ? work_bytes : 4096);
    if (!vm->work_cache) {
        arena_free(vm->eval_cache);
        free(vm);
        return NULL;
    }

    vm->pc = select_first_expr(prog);
    return vm;
}

void svm_destroy(VM* vm) {
    if (!vm) return;
    if (vm->work_cache) arena_free(vm->work_cache);
    if (vm->eval_cache) arena_free(vm->eval_cache);
    free(vm);
}

int svm_eval_atom(VM* vm, SymTab* env, const Token* tok, Value* out) {
    if (!vm || !env || !tok || !out) return SVM_ERR_ARG;

    switch (tok->type) {
        case TOKEN_INTEGER: {
            // Parse integer literal
            char buf[32];
            size_t len = tok->value_len < 31 ? tok->value_len : 31;
            memcpy(buf, tok->value, len);
            buf[len] = '\0';
            out->type = SYM_INTEGER;
            out->val.int_val = atoll(buf);
            return SVM_OK;
        }

        case TOKEN_FLOAT: {
            // Parse float literal
            char buf[32];
            size_t len = tok->value_len < 31 ? tok->value_len : 31;
            memcpy(buf, tok->value, len);
            buf[len] = '\0';
            out->type = SYM_FLOAT;
            out->val.float_val = atof(buf);
            return SVM_OK;
        }

        case TOKEN_STRING: {
            // String literal (skip surrounding quotes)
            // "hello" → hello
            const char* start = tok->value + 1;
            size_t len = tok->value_len - 2;
            char* str = arena_alloc(&vm->eval_cache, len + 1);
            if (!str) return SVM_ERR_OOM;
            memcpy(str, start, len);
            str[len] = '\0';
            out->type = SYM_STRING;
            out->val.str_val = str;
            return SVM_OK;
        }

        case TOKEN_IDENTIFIER: {
            Symbol* sym = symtab_lookup(env, tok->value, tok->value_len);
            if (!sym) {
                fprintf(stderr, "Error: undefined symbol '%.*s'\n", 
                        (int)tok->value_len, tok->value);
                return SVM_ERR_UNBOUND;
            }
            out->type = sym->type;
            out->val = sym->value;
            return SVM_OK;
        }

        case TOKEN_IGNORE:
            return SVM_ERR_EVAL;

        case TOKEN_QUOTE:
        case TOKEN_COMMA:
            fprintf(stderr, "Error: unexpected operator '%.*s' as atom\n",
                    (int)tok->value_len, tok->value);
            return SVM_ERR_EVAL;

        case TOKEN_ERROR:
            fprintf(stderr, "Error: invalid token '%.*s'\n",
                    (int)tok->value_len, tok->value);
            return SVM_ERR_EVAL;

        default:
            fprintf(stderr, "Error: unknown token type %d\n", tok->type);
            return SVM_ERR_EVAL;
    }
}

int svm_step(VM* vm, const ProgramFlux* prog, Vec* stack, size_t root_dest_id, Value* out) {
    if (!vm || !prog || !stack || stack->elem_num == 0) return 0; // done

    Value* cache = (Value*)vm->eval_cache->start;
    size_t slots = vm->eval_cache->capacity / sizeof(Value);

    WorkItem* fr = VEC_TOP_PTR(stack, WorkItem);

    // Sentinel: drive lambda body
    if (fr->expr && fr->expr->id == SIZE_MAX) {
        lambda* fn = fr->fn;
        if (!fn || !fn->body || vec_len(fn->body) == 0) {
            Value res = {0}; res.type = SYM_UNDEFINED;
            cache[fr->dest_id] = res;
            if (fr->dest_id == root_dest_id) *out = res;
            symtab_pop_scope(fr->env);
            VEC_POP(stack);
            return 1;
        }
        size_t blen = vec_len(fn->body);
        if (fr->body_idx < blen) {
            SExpr* form = *(SExpr**)vec_at(fn->body, fr->body_idx);
            fr->body_idx++;
            WorkItem next = { .expr=form, .env=fr->env, .fn=NULL, .body_idx=0, .dest_id=0 };
            if (vec_push_nogrow(stack, &next) != 0) return SVM_ERR_OOM;
            return 1;
        } else {
            SExpr* last = *(SExpr**)vec_at(fn->body, blen - 1);
            if (last->id >= slots || cache[last->id].type == SYM_UNDEFINED) return SVM_ERR_EVAL;
            cache[fr->dest_id] = cache[last->id];
            if (fr->dest_id == root_dest_id) *out = cache[last->id];
            symtab_pop_scope(fr->env);
            VEC_POP(stack);
            return 1;
        }
    }

    SExpr* cur = fr->expr;
    SymTab* env = fr->env;

    if (cache[cur->id].type != SYM_UNDEFINED) {
        if (cur->id == root_dest_id) *out = cache[cur->id];
        VEC_POP(stack);
        return 1;
    }

    // Atom case
    if (cur->start_idx == cur->end_idx) {
        Token t = vec_get_token(cur->tokens, cur->start_idx);
        Value v = {0};
        int rc = svm_eval_atom(vm, env, &t, &v);
        if (rc != SVM_OK) return rc;
        cache[cur->id] = v;
        if (cur->id == root_dest_id) *out = v;
        VEC_POP(stack);
        return 1;
    }

    // Children case
    for (size_t i = cur->start_idx + 1; i <= cur->end_idx; ++i) {
        Token t = vec_get_token(cur->tokens, i);
        if (t.type == TOKEN_IGNORE) continue;
        if (t.s_exprid > cur->id) {
            size_t cid = t.s_exprid;
            if (cid >= slots) return SVM_ERR_EVAL;
            if (cache[cid].type == SYM_UNDEFINED) {
                // find child
                SExpr* child = NULL;
                for (size_t j = 0; j < vec_len(prog->sexprs); ++j) {
                    SExpr* cand = *(SExpr**)vec_at(prog->sexprs, j);
                    if (cand->id == cid) { child = cand; break; }
                }
                if (!child) return SVM_ERR_EVAL;
                WorkItem ch = { .expr=child, .env=env, .fn=NULL, .body_idx=0, .dest_id=0 };
                if (vec_push_nogrow(stack, &ch) != 0) return SVM_ERR_OOM;
                return 1;
            } else {
                SExpr* child = NULL;
                for (size_t j = 0; j < vec_len(prog->sexprs); ++j) {
                    SExpr* cand = *(SExpr**)vec_at(prog->sexprs, j);
                    if (cand->id == cid) { child = cand; break; }
                }
                if (!child) return SVM_ERR_EVAL;
                i = child->end_idx;
            }
        }
    }

    Token op = vec_get_token(cur->tokens, cur->start_idx);
    if (op.type != TOKEN_IDENTIFIER) return SVM_ERR_TYPE;

    Symbol* sym = symtab_lookup(env, op.value, op.value_len);
    if (!sym) sym = symtab_lookup(g_symtab, op.value, op.value_len);
    if (!sym) return SVM_ERR_UNBOUND;

    if (sym->type == SYM_BUILTIN) {
        BuiltinFn fp = (BuiltinFn)sym->value.ptr_val;
        if (!fp) return SVM_ERR_EVAL;
        Value res = {0};
        int rc = fp(vm, env, cur, &res);
        if (rc != SVM_OK) return rc;
        cache[cur->id] = res;
        if (cur->id == root_dest_id) *out = res;
        VEC_POP(stack);
        return 1;
    }

    if (sym->type == SYM_FUNCTION) {
        lambda* fn = sym->value.func_val;
        if (!fn) return SVM_ERR_EVAL;

        SymTab* fn_scope = symtab_push_scope(env);
        if (!fn_scope) return SVM_ERR_OOM;

        size_t nparams = fn->params ? fn->params->elem_num : 0;
        size_t argi = 0;
        for (size_t i = cur->start_idx + 1; i <= cur->end_idx && argi < nparams; ++i) {
            Token t = vec_get_token(cur->tokens, i);
            if (t.type == TOKEN_IGNORE) continue;

            Token p = vec_get_token(fn->params, argi);
            if (p.type != TOKEN_IDENTIFIER) { argi++; continue; }

            Value v = {0};
            if (t.s_exprid == cur->id) {
                int rc = svm_eval_atom(vm, env, &t, &v);
                if (rc != SVM_OK) { symtab_pop_scope(fn_scope); return rc; }
            } else if (t.s_exprid > cur->id) {
                if (t.s_exprid >= slots || cache[t.s_exprid].type == SYM_UNDEFINED) {
                    symtab_pop_scope(fn_scope); return SVM_ERR_EVAL;
                }
                v = cache[t.s_exprid];
            } else {
                continue;
            }
            symtab_define(fn_scope, p.value, p.value_len, v.type, v.val, SYM_FLAG_MUTABLE);
            argi++;
        }

        SExpr* sentinel = (SExpr*)arena_alloc(&vm->work_cache, sizeof(SExpr));
        if (!sentinel) { symtab_pop_scope(fn_scope); return SVM_ERR_OOM; }
        sentinel->id = SIZE_MAX; sentinel->start_idx = 0; sentinel->end_idx = 0; sentinel->tokens = NULL;

        WorkItem body = { .expr=sentinel, .env=fn_scope, .fn=fn, .body_idx=0, .dest_id=cur->id };
        VEC_POP(stack);
        if (vec_push_nogrow(stack, &body) != 0) { symtab_pop_scope(fn_scope); return SVM_ERR_OOM; }
        return 1;
    }

    return SVM_ERR_TYPE;
}


int svm_eval_expr(VM* vm, SymTab* locals, const ProgramFlux* prog, SExpr* expr, Value* out) {
    if (!vm || !prog || !expr || !out) return SVM_ERR_ARG;

    Value* cache = (Value*)vm->eval_cache->start;
    if (cache[expr->id].type != SYM_UNDEFINED) { *out = cache[expr->id]; return SVM_OK; }

    size_t cap = vec_len(prog->sexprs);
    Vec* stack = arena_vec_new(&vm->work_cache, sizeof(WorkItem), cap * 2 + 8);
    if (!stack) return SVM_ERR_OOM;

    WorkItem root = {
        .expr     = expr,
        .env      = locals ? locals : g_symtab,
        .fn       = NULL,
        .body_idx = 0,
        .dest_id  = expr->id
    };
    if (vec_push_nogrow(stack, &root) != 0) return SVM_ERR_OOM;

    for (;;) {
        int r = svm_step(vm, prog, stack, expr->id, out);
        if (r < 0) return r;  // error
        if (r == 0) break;    // done
    }
    return SVM_OK;
}

int evaluate_program(ProgramFlux* program) {
    if (!program) {
        fprintf(stderr, "Error: No program to evaluate\n");
        return -1;
    }
    Arena* global_arena = arena_create(8192);
    if (!global_arena) {
        fprintf(stderr, "Error: Failed to create global arena\n");
        return -1;
    }
    SymTab* global_env = symtab_new(32, NULL, global_arena);
    if (!global_env) {
        fprintf(stderr, "Error: Failed to create global symbol table\n");
        arena_free(global_arena);
        return -1;
    }
    if (register_arithmetic_builtins(global_env) != 0) {
        fprintf(stderr, "Error: Failed to register arithmetic builtins\n");
        symtab_destroy(&global_env, false);
        arena_free(global_arena);
        return -1;
    }

    // Create VM
    VM* vm = svm_create(program);
    if (!vm) {
        fprintf(stderr, "Error: Failed to create VM\n");
        symtab_destroy(&global_env, false);
        arena_free(global_arena);
        return -1;
    }
    printf("\n=== Evaluation Results ===\n");
    for (size_t expr_idx = 0; expr_idx < vec_len(program->sexprs); expr_idx++) {
        SExpr* sexpr = *(SExpr**)vec_at(program->sexprs, expr_idx);
        bool has_content = false;
        for (size_t i = sexpr->start_idx; i <= sexpr->end_idx; i++) {
            Token tok = vec_get_token(program->tokens, i);
            if (tok.type != TOKEN_IGNORE) {
                has_content = true;
                break;
            }
        }
        if (!has_content) {
            continue;
        }
        printf("Evaluating: ");
        for (size_t i = sexpr->start_idx; i <= sexpr->end_idx; i++) {
            Token tok = vec_get_token(program->tokens, i);
            if (tok.type != TOKEN_IGNORE) {
                printf("%.*s ", (int)tok.value_len, tok.value);
            }
        }
        printf("→ ");
        Value result;
        int eval_result = svm_eval_expr(vm, NULL, program, sexpr, &result);
        if (eval_result == SVM_OK) {
            print_value(&result);
            printf("\n");
        } else {
            printf("ERROR: Evaluation failed with code %d\n", eval_result);
        }
    }
    printf("=== End Evaluation ===\n\n");
    svm_destroy(vm);
    symtab_destroy(&global_env, false);
    arena_free(global_arena);
    return 0;
}

/* ------------------------- Builtins ----------------------- */

int bltn_add(VM* vm, SymTab* env, const SExpr* call, Value* out) {
    if (!vm || !call || !out) return SVM_ERR_ARG;
    Value* cache = (Value*)vm->eval_cache->start;
    size_t slots = vm->eval_cache->capacity / sizeof(Value);
    Vec* args = arena_vec_new(&vm->work_cache, sizeof(Value), 16);
    if (!args) return SVM_ERR_OOM;
    for (size_t i = call->start_idx + 1; i <= call->end_idx; ++i) {
        Token t = vec_get_token(call->tokens, i);
        if (t.type == TOKEN_IGNORE) continue;
        if (t.s_exprid > call->id) {
            if (t.s_exprid >= slots || cache[t.s_exprid].type == SYM_UNDEFINED) {
                return SVM_ERR_EVAL;
            }
            vec_push(&args, &cache[t.s_exprid]);
            SExpr* child = NULL;
            for (size_t j = 0; j < vec_len(vm->pc->tokens); ++j) {
                SExpr* cand = *(SExpr**)vec_at(vm->pc->tokens, j);
                if (cand->id == t.s_exprid) { 
                    child = cand; 
                    i = child->end_idx;
                    break;
                }
            }
        } else {
            Value atom_val;
            int rc = svm_eval_atom(vm, env, &t, &atom_val);
            if (rc != SVM_OK) return rc;
            vec_push(&args, &atom_val);
        }
    }
    if (vec_len(args) == 0) {
        out->type = SYM_INTEGER;
        out->val.int_val = 0;
        return SVM_OK;
    }
    bool all_int = true;
    for (size_t i = 0; i < vec_len(args); ++i) {
        Value* arg = (Value*)vec_at(args, i);
        if (arg->type != SYM_INTEGER && arg->type != SYM_FLOAT) {
            return SVM_ERR_TYPE;
        }
        if (arg->type == SYM_FLOAT) {
            all_int = false;
        }
    }
    if (all_int) {
        long long result = 0;
        for (size_t i = 0; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            result += arg->val.int_val;
        }
        out->type = SYM_INTEGER;
        out->val.int_val = result;
    } else {
        double result = 0.0;
        for (size_t i = 0; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            if (arg->type == SYM_INTEGER) {
                result += (double)arg->val.int_val;
            } else {
                result += arg->val.float_val;
            }
        }
        out->type = SYM_FLOAT;
        out->val.float_val = result;
    }
    return SVM_OK;
}

int bltn_sub(VM* vm, SymTab* env, const SExpr* call, Value* out) {
    if (!vm || !call || !out) return SVM_ERR_ARG;
    Value* cache = (Value*)vm->eval_cache->start;
    size_t slots = vm->eval_cache->capacity / sizeof(Value);
    Vec* args = arena_vec_new(&vm->work_cache, sizeof(Value), 16);
    if (!args) return SVM_ERR_OOM;
    for (size_t i = call->start_idx + 1; i <= call->end_idx; ++i) {
        Token t = vec_get_token(call->tokens, i);
        if (t.type == TOKEN_IGNORE) continue;
        if (t.s_exprid > call->id) {
            if (t.s_exprid >= slots || cache[t.s_exprid].type == SYM_UNDEFINED) {
                return SVM_ERR_EVAL;
            }
            vec_push(&args, &cache[t.s_exprid]);
            // Skip the sub-expression tokens
            SExpr* child = NULL;
            for (size_t j = 0; j < vec_len(vm->pc->tokens); ++j) {
                SExpr* cand = *(SExpr**)vec_at(vm->pc->tokens, j);
                if (cand->id == t.s_exprid) { 
                    child = cand; 
                    i = child->end_idx;
                    break;
                }
            }
        } else {
            Value atom_val;
            int rc = svm_eval_atom(vm, env, &t, &atom_val);
            if (rc != SVM_OK) return rc;
            vec_push(&args, &atom_val);
        }
    }
    if (vec_len(args) == 0) {
        out->type = SYM_INTEGER;
        out->val.int_val = 0;
        return SVM_OK;
    }
    if (vec_len(args) == 1) {
        // Unary minus
        Value* arg = (Value*)vec_at(args, 0);
        if (arg->type == SYM_INTEGER) {
            out->type = SYM_INTEGER;
            out->val.int_val = -arg->val.int_val;
        } else if (arg->type == SYM_FLOAT) {
            out->type = SYM_FLOAT;
            out->val.float_val = -arg->val.float_val;
        } else {
            return SVM_ERR_TYPE;
        }
        return SVM_OK;
    }
    bool all_int = true;
    for (size_t i = 0; i < vec_len(args); ++i) {
        Value* arg = (Value*)vec_at(args, i);
        if (arg->type != SYM_INTEGER && arg->type != SYM_FLOAT) {
            return SVM_ERR_TYPE;
        }
        if (arg->type == SYM_FLOAT) {
            all_int = false;
        }
    }
    if (all_int) {
        long long result = ((Value*)vec_at(args, 0))->val.int_val;
        for (size_t i = 1; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            result -= arg->val.int_val;
        }
        out->type = SYM_INTEGER;
        out->val.int_val = result;
    } else {
        double result;
        Value* first = (Value*)vec_at(args, 0);
        if (first->type == SYM_INTEGER) {
            result = (double)first->val.int_val;
        } else {
            result = first->val.float_val;
        }
        for (size_t i = 1; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            if (arg->type == SYM_INTEGER) {
                result -= (double)arg->val.int_val;
            } else {
                result -= arg->val.float_val;
            }
        }
        out->type = SYM_FLOAT;
        out->val.float_val = result;
    }
    return SVM_OK;
}

int bltn_mul(VM* vm, SymTab* env, const SExpr* call, Value* out) {
    if (!vm || !call || !out) return SVM_ERR_ARG;
    Value* cache = (Value*)vm->eval_cache->start;
    size_t slots = vm->eval_cache->capacity / sizeof(Value);
    Vec* args = arena_vec_new(&vm->work_cache, sizeof(Value), 16);
    if (!args) return SVM_ERR_OOM;
    for (size_t i = call->start_idx + 1; i <= call->end_idx; ++i) {
        Token t = vec_get_token(call->tokens, i);
        if (t.type == TOKEN_IGNORE) continue;
        if (t.s_exprid > call->id) {
            if (t.s_exprid >= slots || cache[t.s_exprid].type == SYM_UNDEFINED) {
                return SVM_ERR_EVAL;
            }
            vec_push(&args, &cache[t.s_exprid]);
            // Skip the sub-expression tokens
            SExpr* child = NULL;
            for (size_t j = 0; j < vec_len(vm->pc->tokens); ++j) {
                SExpr* cand = *(SExpr**)vec_at(vm->pc->tokens, j);
                if (cand->id == t.s_exprid) { 
                    child = cand; 
                    i = child->end_idx;
                    break;
                }
            }
        } else {
            Value atom_val;
            int rc = svm_eval_atom(vm, env, &t, &atom_val);
            if (rc != SVM_OK) return rc;
            vec_push(&args, &atom_val);
        }
    }
    if (vec_len(args) == 0) {
        out->type = SYM_INTEGER;
        out->val.int_val = 1;
        return SVM_OK;
    }
    bool all_int = true;
    for (size_t i = 0; i < vec_len(args); ++i) {
        Value* arg = (Value*)vec_at(args, i);
        if (arg->type != SYM_INTEGER && arg->type != SYM_FLOAT) {
            return SVM_ERR_TYPE;
        }
        if (arg->type == SYM_FLOAT) {
            all_int = false;
        }
    }
    if (all_int) {
        long long result = 1;
        for (size_t i = 0; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            result *= arg->val.int_val;
        }
        out->type = SYM_INTEGER;
        out->val.int_val = result;
    } else {
        double result = 1.0;
        for (size_t i = 0; i < vec_len(args); ++i) {
            Value* arg = (Value*)vec_at(args, i);
            if (arg->type == SYM_INTEGER) {
                result *= (double)arg->val.int_val;
            } else {
                result *= arg->val.float_val;
            }
        }
        out->type = SYM_FLOAT;
        out->val.float_val = result;
    }
    return SVM_OK;
}

int bltn_div(VM* vm, SymTab* env, const SExpr* call, Value* out) {
    if (!vm || !call || !out) return SVM_ERR_ARG;

    Value* cache = (Value*)vm->eval_cache->start;
    size_t slots = vm->eval_cache->capacity / sizeof(Value);

    Vec* args = arena_vec_new(&vm->work_cache, sizeof(Value), 16);
    if (!args) return SVM_ERR_OOM;
 
    for (size_t i = call->start_idx + 1; i <= call->end_idx; ++i) {
        Token t = vec_get_token(call->tokens, i);
        if (t.type == TOKEN_IGNORE) continue;

        if (t.s_exprid > call->id) {
            if (t.s_exprid >= slots || cache[t.s_exprid].type == SYM_UNDEFINED) {
                return SVM_ERR_EVAL;
            }
            vec_push(&args, &cache[t.s_exprid]);
            // Skip the sub-expression tokens
            SExpr* child = NULL;
            for (size_t j = 0; j < vec_len(vm->pc->tokens); ++j) {
                SExpr* cand = *(SExpr**)vec_at(vm->pc->tokens, j);
                if (cand->id == t.s_exprid) { 
                    child = cand; 
                    i = child->end_idx;
                    break;
                }
            }
        } else {
            Value atom_val;
            int rc = svm_eval_atom(vm, env, &t, &atom_val);
            if (rc != SVM_OK) return rc;
            vec_push(&args, &atom_val);
        }
    }

    if (vec_len(args) == 0) {
        return SVM_ERR_ARG;
    }

    if (vec_len(args) == 1) {
        // Reciprocal
        Value* arg = (Value*)vec_at(args, 0);
        if (arg->type == SYM_INTEGER) {
            if (arg->val.int_val == 0) return SVM_ERR_EVAL;
            out->type = SYM_FLOAT;
            out->val.float_val = 1.0 / (double)arg->val.int_val;
        } else if (arg->type == SYM_FLOAT) {
            if (arg->val.float_val == 0.0) return SVM_ERR_EVAL;
            out->type = SYM_FLOAT;
            out->val.float_val = 1.0 / arg->val.float_val;
        } else {
            return SVM_ERR_TYPE;
        }
        return SVM_OK;
    }

    for (size_t i = 0; i < vec_len(args); ++i) {
        Value* arg = (Value*)vec_at(args, i);
        if (arg->type != SYM_INTEGER && arg->type != SYM_FLOAT) {
            return SVM_ERR_TYPE;
        }
        if (i > 0) {
            if ((arg->type == SYM_INTEGER && arg->val.int_val == 0) ||
                (arg->type == SYM_FLOAT && arg->val.float_val == 0.0)) {
                return SVM_ERR_EVAL;
            }
        }
    }

    double result;
    Value* first = (Value*)vec_at(args, 0);
    if (first->type == SYM_INTEGER) {
        result = (double)first->val.int_val;
    } else {
        result = first->val.float_val;
    }

    for (size_t i = 1; i < vec_len(args); ++i) {
        Value* arg = (Value*)vec_at(args, i);
        if (arg->type == SYM_INTEGER) {
            result /= (double)arg->val.int_val;
        } else {
            result /= arg->val.float_val;
        }
    }

    out->type = SYM_FLOAT;
    out->val.float_val = result;
    return SVM_OK;
}


// int bltn_lambda(VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_mapcar(VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_quote (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_atom  (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_null  (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_cons  (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_car   (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
// 
// int bltn_cdr   (VM* vm, SymTab* env, const SExpr* call, Value* out) {
// 
// }
