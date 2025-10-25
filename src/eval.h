/*
 * @file eval.h
 */
#ifndef EVAL_H
#define EVAL_H


#include "builtins.h"
#include "parser.h"
#include "arena.h"

#include <stdint.h>

typedef enum BuiltinType : uint8_t {
    BUILTIN_ADD,
    BUILTIN_SUB,
    BUILTIN_MUL,
    BUILTIN_DIV,
    BUILTIN_MOD,
    BUILTIN_NUM_EQ,
    BUILTIN_LESS_THAN,
    BUILTIN_GREATER_THAN,
    BUILTIN_LESS_EQ,
    BUILTIN_GREATER_EQ,
    BUILTIN_CONS,
    BUILTIN_CAR,
    BUILTIN_CDR,
    BUILTIN_LIST,
    BUILTIN_IS_ATOM,
    BUILTIN_IS_PAIR,
    BUILTIN_IS_LIST,
    BUILTIN_IS_NULL,
    BUILTIN_IS_NUMBER,
    BUILTIN_IS_STRING,
    BUILTIN_IS_SYMBOL,
    BUILTIN_IS_PROCEDURE,
    BUILTIN_EQ,
    BUILTIN_EQUAL,
    BUILTIN_DISPLAY,
    BUILTIN_NEWLINE,
    BUILTIN_APPLY,
    BUILTIN_EVAL,
    BUILTIN_EXIT
} BuiltinType;

typedef Cons* (*builtin_fn)(ConsList* args, Arena** arena);

#endif
