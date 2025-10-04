/*
 * @file symtab.c
 * @brief A symbol table implemented using the Vec.h API.
*/
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "symtab.h"
#include "src/ggc.h"
#include "vec.h"


/* -------------------- Functions to get References -------------------- */

void*** get_reference_from_cons(void* obj, size_t* count) {
    if (!obj || !count) return NULL;

    Symbol* sym = (Symbol*)obj;
    static void** refs[T_CONS_REF_NUM];
    *count = T_CONS_REF_NUM;

    ConsData* cons_cell = (ConsData*)sym->value.value_ptr;

    refs[0] = (void**)&cons_cell->car;
    refs[1] = (void**)&cons_cell->cdr;

    return (void***)refs;
}

void*** get_reference_from_symbol(void* obj, size_t* count) {
    if (!obj || !count) return NULL;

    Symbol* sym = (Symbol*)obj;
    static void** refs[T_SYM_REF_NUM];
    *count = T_SYM_REF_NUM;

    SymData* nested_sym = (SymData*)sym->value.value_ptr;

    refs[0] = (void**)&nested_sym->sym;

    return (void***)refs;
}

void*** get_reference_from_function(void* obj, size_t* count) {
    if (!obj || !count) return NULL;

    Symbol* sym = (Symbol*)obj;
    static void** refs[T_FUNC_REF_NUM];
    *count = T_FUNC_REF_NUM;

    FunctionData* func_ptr = (FunctionData*)sym->value.value_ptr;

    refs[0] = (void**)&func_ptr->params;
    if (func_ptr->is_c_func) {
        refs[1] = (void**)&func_ptr->func.c.fn;
        refs[2] = (void**)&func_ptr->func.c.c_name;
    } else {
        refs[1] = (void**)&func_ptr->func.lisp.body;
        refs[2] = (void**)&func_ptr->func.lisp.closure_env;
    }

    return (void***)refs;
}

void*** get_reference_from_macro(void* obj, size_t* count) {
    if (!obj || !count) return NULL;

    Symbol* sym = (Symbol*)obj;
    static void** refs[T_MACRO_REF_NUM];
    *count = T_MACRO_REF_NUM;

    MacroData* macro_ptr = (MacroData*)sym->value.value_ptr;

    refs[0] = (void**)&macro_ptr->body;
    refs[1] = (void**)&macro_ptr->params;

    return (void***)refs;
}

extract_reference reference_extractors[] = {
    [T_NIL]          = NULL,                        ///< Immediate value, no refs
    [T_CONS]         = get_reference_from_cons,     ///< Has car & cdr
    [T_SYMBOL]       = get_reference_from_symbol,   ///< Has value, plist, name
    [T_INT]          = NULL,                        ///< Immediate value, no refs
    [T_STRING]       = NULL,                        ///< Just char data, no refs
    [T_FLOAT]        = NULL,                        ///< Just numeric data, no refs
    [T_BOOL]         = NULL,                        ///< Immediate value, no refs
    [T_FUNCTION]     = get_reference_from_function, ///< Has env, body, params
    [T_MACRO]        = get_reference_from_macro,    ///< Has body, env
};

void*** extract_sym_references(void* obj, size_t* count) {
    if (!obj || !count) return NULL;
    Symbol* sym = (Symbol*)obj;

    static void** all_refs[MAX_REF];  // Max possible refs (generic + type-specific)
    size_t total_count = 0;

    if (sym->env_ptr) {
        all_refs[total_count++] = (void**)&sym->env_ptr;
    }

    extract_reference extractor = reference_extractors[sym->type];
    if (extractor) {
        size_t type_refs_count;
        void*** type_refs = extractor(obj, &type_refs_count);  // Pass whole Symbol, not value_ptr!

        if (type_refs) {
            for (size_t i = 0; i < type_refs_count; i++) {
                all_refs[total_count++] = type_refs[i];
            }
        }
    }

    *count = total_count;
    return total_count > 0 ? (void***)all_refs : NULL;
}

/* ----------------------- Alloc functions ----------------------- */

Symbol* alloc_cons(Gc* gc, Symbol* car, Symbol* cdr) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_CONS;

    ConsData* cons = gc_alloc(gc, sizeof(ConsData), NULL);
    cons->car = car;
    cons->cdr = cdr;

    sym->value.value_ptr = cons;
    return sym;
}

Symbol* alloc_lisp_function(Gc* gc, Symbol* params, Symbol* body, EnvFrame* env) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_FUNCTION;

    FunctionData* fn = gc_alloc(gc, sizeof(FunctionData), NULL);
    fn->params = params;
    fn->is_c_func = false;
    fn->func.lisp.body = body;
    fn->func.lisp.closure_env = env;

    sym->value.value_ptr = fn;
    sym->env_ptr = env;
    return sym;
}


Symbol* alloc_c_function_full(Gc* gc, CFunctionFn c_fn, const char* name,
                              Symbol* param_names, int min_arity, int max_arity){
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_FUNCTION;

    FunctionData* fn = gc_alloc(gc, sizeof(FunctionData), NULL);
    fn->params = param_names;
    fn->is_c_func = true;
    fn->func.c.fn = c_fn;
    fn->func.c.c_name = name;
    fn->arity.max_arity = max_arity;
    fn->arity.min_arity = min_arity;

    sym->value.value_ptr = fn;
    sym->env_ptr = NULL;
    return sym;
}

Symbol* alloc_sym_base(Gc* gc, char* name, TypeTag type, EvalKind evalkind, 
                       uint32_t flags, EnvFrame* env) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_SYMBOL;

    SymData* inner_sym = gc_alloc(gc, sizeof(SymData), NULL);

    inner_sym->sym->name = name;
    inner_sym->sym->len = strlen(name);
    inner_sym->sym->hash = hash(name);
    inner_sym->sym->type = type;
    inner_sym->sym->flags = flags;
    inner_sym->sym->eval = evalkind;
    inner_sym->sym->env_ptr = env;

    sym->value.value_ptr = inner_sym;
    sym->env_ptr = NULL;

    return sym; ///< inner_sym value is not set to let the caller choose which type of value to use.
                ///< @note use just sym->value.value_ptr->sym->value.(The type you want) = value.
}

Symbol* alloc_int(Gc* gc, int32_t val) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_INT;

    sym->value.int_val = val;

    return sym;
}

Symbol* alloc_float(Gc* gc, double val) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_FLOAT;

    sym->value.int_val = val;

    return sym;
}

Symbol* alloc_bool(Gc* gc, bool val) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_BOOL;

    sym->value.int_val = val;

    return sym;
}

Symbol alloc_string(Gc* gc, char* str, size_t len) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_STRING;

    StringData* string = gc_alloc(gc, len, NULL);

    memcpy(string->str, str, len);

}

Symbol* alloc_macro(Gc* gc, Symbol* params, Symbol* body) {
    Symbol* sym = gc_alloc(gc, sizeof(Symbol), NULL);
    sym->type = T_CONS;

    MacroData* macro = gc_alloc(gc, sizeof(MacroData), NULL);

    macro->body = body;
    macro->params = params;

    sym->value.value_ptr = macro;
    return sym;
}

/* -------------------- Symbol Table Lifetime -------------------- */

SymTab* symtab_create(size_t initial_capacity) {
    return vec_new(initial_capacity, sizeof(Symbol*));
}

void symtab_destroy(SymTab** st) {
	vec_free(st);
    *st = NULL;
}

/* -------------------- Symbol Table API (Main Interface) -------------------- */

Symbol* symtab_intern(Gc* gc, SymTab* st, char* name) {
    if (!st) return NULL;

    size_t sym_size = sizeof(Symbol) + strlen(name);

    Symbol* sym = (Symbol*)gc_alloc(gc, sym_size, NULL);
    if (!sym) return NULL;

    sym->name  = strdup(name);
    sym->len   = 0;
    sym->hash  = hash(name);

    sym->type  = T_NIL;
    sym->eval  = EK_DATA;
    sym->flags = NO_FLAGS;

    sym->value.value_ptr = NULL;
    sym->env_ptr   = NULL;

    sym->gcinfo.gen      = 0;
    sym->gcinfo.age      = 0;
    sym->gcinfo.obj_size = sym_size;

    if (st->elem_num < st->maxcap) {
        char* slot = (char*)(st + 1) + (st->elem_num * st->elem_size);
        memcpy(slot, &sym, st->elem_size);
        st->elem_num++;
        st->bump_ptr = (char*)(st + 1) + (st->elem_num * st->elem_size);
    } else {
        if (vec_push(&st, &sym) != 0) {
            free(sym);
            return NULL;
        }
    }

    return sym;
}

Symbol* symtab_lookup(SymTab* st, const char* name) {
    uint32_t sym_hash = hash(name);

    for (size_t i = 0; i < st->elem_num; i++) {
        Symbol* sym = vec_at(st, i);
        if (sym->hash == sym_hash 
			&& strcmp(sym->name, name) == 0) {
            return sym;
        }
    }

    return NULL;
}
