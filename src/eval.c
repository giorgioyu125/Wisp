/*
 * @file eval.c
 */
#include "eval.h"
#include "builtins.h"
#include "parser.h"


Cons* builtin_add(ConsList* args, Arena** arena);
Cons* builtin_sub(ConsList* args, Arena** arena);
Cons* builtin_mul(ConsList* args, Arena** arena);
Cons* builtin_div(ConsList* args, Arena** arena);
Cons* builtin_mod(ConsList* args, Arena** arena);
Cons* builtin_num_eq(ConsList* args, Arena** arena);
Cons* builtin_less_than(ConsList* args, Arena** arena);
Cons* builtin_greater_than(ConsList* args, Arena** arena);
Cons* builtin_less_eq(ConsList* args, Arena** arena);
Cons* builtin_greater_eq(ConsList* args, Arena** arena);
Cons* builtin_cons(ConsList* args, Arena** arena);
Cons* builtin_car(ConsList* args, Arena** arena);
Cons* builtin_cdr(ConsList* args, Arena** arena);
Cons* builtin_list(ConsList* args, Arena** arena);
Cons* builtin_is_atom(ConsList* args, Arena** arena);
Cons* builtin_is_pair(ConsList* args, Arena** arena);
Cons* builtin_is_list(ConsList* args, Arena** arena);
Cons* builtin_is_null(ConsList* args, Arena** arena);
Cons* builtin_is_number(ConsList* args, Arena** arena);
Cons* builtin_is_string(ConsList* args, Arena** arena);
Cons* builtin_is_symbol(ConsList* args, Arena** arena);
Cons* builtin_is_procedure(ConsList* args, Arena** arena);
Cons* builtin_eq(ConsList* args, Arena** arena);
Cons* builtin_equal(ConsList* args, Arena** arena);
Cons* builtin_display(ConsList* args, Arena** arena);
Cons* builtin_newline(ConsList* args, Arena** arena);
Cons* builtin_apply(ConsList* args, Arena** arena);
Cons* builtin_eval(ConsList* args, Arena** arena);
Cons* builtin_exit(ConsList* args, Arena** arena);

static const builtin_fn builtin_dispatch_table[] = {
    [BUILTIN_ADD]           = builtin_add,
    [BUILTIN_SUB]           = builtin_sub,
    [BUILTIN_MUL]           = builtin_mul,
    [BUILTIN_DIV]           = builtin_div,
    [BUILTIN_MOD]           = builtin_mod,
    [BUILTIN_NUM_EQ]        = builtin_num_eq,
    [BUILTIN_LESS_THAN]     = builtin_less_than,
    [BUILTIN_GREATER_THAN]  = builtin_greater_than,
    [BUILTIN_LESS_EQ]       = builtin_less_eq,
    [BUILTIN_GREATER_EQ]    = builtin_greater_eq,
    [BUILTIN_CONS]          = builtin_cons,
    [BUILTIN_CAR]           = builtin_car,
    [BUILTIN_CDR]           = builtin_cdr,
    [BUILTIN_LIST]          = builtin_list,
    [BUILTIN_IS_ATOM]       = builtin_is_atom,
    [BUILTIN_IS_PAIR]       = builtin_is_pair,
    [BUILTIN_IS_LIST]       = builtin_is_list,
    [BUILTIN_IS_NULL]       = builtin_is_null,
    [BUILTIN_IS_NUMBER]     = builtin_is_number,
    [BUILTIN_IS_STRING]     = builtin_is_string,
    [BUILTIN_IS_SYMBOL]     = builtin_is_symbol,
    [BUILTIN_IS_PROCEDURE]  = builtin_is_procedure,
    [BUILTIN_EQ]            = builtin_eq,
    [BUILTIN_EQUAL]         = builtin_equal,
    [BUILTIN_DISPLAY]       = builtin_display,
    [BUILTIN_NEWLINE]       = builtin_newline,
    [BUILTIN_APPLY]         = builtin_apply,
    [BUILTIN_EVAL]          = builtin_eval,
    [BUILTIN_EXIT]          = builtin_exit,
};
