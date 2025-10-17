/*
 * @file parser.h
 */
#ifndef PARSER_H
#define PARSER_H

#include "ll.h"
#include "vec.h"

#include <stdalign.h>

/* ------------ API ------------ */

LinkedList* ast_init(size_t starting_elements);

LinkedList* construct_ast(Vec* Tokens);

#endif
