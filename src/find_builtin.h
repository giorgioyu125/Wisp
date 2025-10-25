/*
 * @file find_builtin.h
 */
#ifndef BUILTINS_H
#define BUILTINS_H

#include <stddef.h>
#include <stdio.h>
#include "eval.h"

struct BuiltinName {
    const char* name;
    BuiltinType type;
};

const struct BuiltinName* find_builtin(const char* str, size_t len);

#endif
