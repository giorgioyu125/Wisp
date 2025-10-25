/*
* @file parser.c
*/

#include "parser.h"
#include "lexer.h"
#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#define MAX_CONSECUTIVE_QUOTES 8 ///< This is the maximum number of nested quotes ''''...(value)

/* ------------ Utilities (Pure Cons-Cell Logic) ------------ */

Cons* make_atom(const void* data, size_t size, NodeType type, Arena** arena) {
    Cons* c = arena_alloc(arena, sizeof(Cons) + size);
    if (!c) return NULL;

    c->type = type;
    c->cdr = NULL;
    c->size = size;
    memcpy(c->car, data, size);
    return c;
}

Cons* parse_list(Cons** cursor, Arena** arena) {
    if (!cursor || !*cursor || (*cursor)->type != NODE_OPENING_SEPARATOR) return NULL;

    Cons* cur = (*cursor)->cdr;

    ConsList* body = arena_alloc(arena, sizeof(ConsList));
    if (!body) return NULL;
    cons_list_init(body);

    NodeType quote_stack[MAX_CONSECUTIVE_QUOTES];
    int quote_stack_top = 0;

    while (cur) {
        if (cur->type >= NODE_QUOTE && cur->type <= NODE_UNQUOTE) {
            if (quote_stack_top < 8) {
                quote_stack[quote_stack_top++] = cur->type;
            }
            cur = cur->cdr;
            continue;
        }

        Cons* expr = NULL;

        if (cur->type == NODE_OPENING_SEPARATOR) {
            expr = parse_list(&cur, arena);
            if (!expr) return NULL;
        } else if (cur->type == NODE_CLOSING_SEPARATOR) {
            *cursor = cur->cdr;
            return wrap_list(body, arena);
        } else {
            expr = cons_clone_shallow(cur, arena);
            if (!expr) return NULL;
            cur = cur->cdr;
        }

        while (quote_stack_top > 0) {
            Cons* temp = expr;
            expr = make_atom(&temp, sizeof(Cons*), quote_stack[--quote_stack_top], arena);
            if (!expr) return NULL;
        }

        cons_list_push_back(body, expr);
    }

    fprintf(stderr, "Parsing Error: Unclosed parenthesis.\n");
    return NULL;
}


/* --------------- Parser API ----------------- */

ConsList* parse_sexpr(Vec* tokens, Arena** arena) {
    if (!tokens || !arena || !*arena || vec_len(tokens) == 0) {
        return NULL;
    }

    Cons* head = NULL;
    Cons* tail = NULL;

    const size_t ntok = vec_len(tokens);
    for (size_t i = 0; i < ntok; i++) {
        Token tok = vec_get_token(tokens, i);
        if (tok.type == TOKEN_IGNORE) continue;

        Cons* node = NULL;

        switch (tok.type) {
            case TOKEN_LPAREN: {
                node = make_shallow_atom(NODE_OPENING_SEPARATOR, NULL, 0, arena);
            } break;

            case TOKEN_RPAREN: {
                node = make_shallow_atom(NODE_CLOSING_SEPARATOR, NULL, 0, arena);
            } break;

            case TOKEN_INTEGER: {
                char buf[32];
                size_t len = tok.value_len < 31 ? tok.value_len : 31;
                memcpy(buf, tok.value, len);
                buf[len] = '\0';

                errno = 0;
                char* endptr = NULL;
                long long val = strtoll(buf, &endptr, 10);

                if (errno == ERANGE || endptr == buf || *endptr != '\0') {
                    char* s = (char*)arena_alloc(arena, tok.value_len + 1);
                    if (!s) return NULL;
                    memcpy(s, tok.value, tok.value_len);
                    s[tok.value_len] = '\0';
                    node = make_atom(&s, sizeof(char*), NODE_ATOM_SYM, arena);
                } else {
                    node = make_atom(&val, sizeof(val), NODE_ATOM_INT, arena);
                }
            } break;

            case TOKEN_FLOAT: {
                char buf[64];
                size_t len = tok.value_len < 63 ? tok.value_len : 63;
                memcpy(buf, tok.value, len);
                buf[len] = '\0';

                errno = 0;
                char* endptr = NULL;
                long double val = strtold(buf, &endptr);

                if (errno == ERANGE || endptr == buf || *endptr != '\0' || !isfinite(val)) {
                    char* s = (char*)arena_alloc(arena, tok.value_len + 1);
                    if (!s) return NULL;
                    memcpy(s, tok.value, tok.value_len);
                    s[tok.value_len] = '\0';
                    node = make_atom(&s, sizeof(char*), NODE_ATOM_SYM, arena);
                } else {
                    node = make_atom(&val, sizeof(long double), NODE_ATOM_FLOAT, arena);
                }
            } break;

            case TOKEN_STRING: {
                const char* start = tok.value;
                size_t len = tok.value_len;
                if (len >= 2 && start[0] == '"' && start[len - 1] == '"') {
                    start++; len -= 2;
                }
                char* s = (char*)arena_alloc(arena, len + 1);
                if (!s) {
                    return NULL;
                }
                memcpy(s, start, len);
                s[len] = '\0';
                node = make_atom(&s, sizeof(char*), NODE_ATOM_STR, arena);
            } break;


            case TOKEN_UNINTERNED_SYMBOL: {
                char* s = (char*)arena_alloc(arena, tok.value_len + 1);
                if (!s) return NULL;
                memcpy(s, tok.value, tok.value_len);
                s[tok.value_len] = '\0';
                node = make_atom(&s, sizeof(char*), NODE_ATOM_UNINTERNED, arena);
            } break;

            case TOKEN_QUOTE:{
                node = make_shallow_atom(NODE_QUOTE, NULL, 0, arena);
            }break;

            case TOKEN_BACKQUOTE:{
                node = make_shallow_atom(NODE_QUASIQUOTE, NULL, 0, arena);
            } break;

            case TOKEN_COMMA:{
                node = make_shallow_atom(NODE_UNQUOTE, NULL, 0, arena);
            }break;

            case TOKEN_IDENTIFIER: {
                char* s = (char*)arena_alloc(arena, tok.value_len + 1);
                if (!s) {
                    return NULL;
                }
                memcpy(s, tok.value, tok.value_len);
                s[tok.value_len] = '\0';
                node = make_atom(&s, sizeof(char*), NODE_ATOM_SYM, arena);
            } break;

            case TOKEN_ERROR:
            default: {
                return NULL;
            }
        }

        if (!node) {
            return NULL;
        }
        if (!head) {
            head = tail = node;
        } else {
            tail->cdr = node;
            tail = node;
        }
    }

    if (!head) {
        return NULL;
    }

    Cons* cursor = head;
    ConsList* result = (ConsList*)arena_alloc(arena, sizeof(ConsList));
    if (!result) {
        return NULL;
    }
    cons_list_init(result);

    while (cursor) {
        Cons* expr = NULL;

        if (cursor->type == NODE_OPENING_SEPARATOR) {
            expr = parse_list(&cursor, arena);
            if (!expr) {
                return NULL;
            }
        } else if (cursor->type == NODE_CLOSING_SEPARATOR) {
            return NULL;
        } else {
            expr = cons_clone_shallow(cursor, arena);
            if (!expr) {
                return NULL;
            }
            cursor = cursor->cdr;
        }

        cons_list_push_back(result, expr);
    }

    return result;
}

ConsList* parse_program(Vec* tokens, Arena** arena) {
    if (!tokens || !arena || !*arena) return NULL;
    if (vec_len(tokens) == 0) {
        ConsList* empty = (ConsList*)arena_alloc(arena, sizeof(ConsList));
        if (!empty) return NULL;
        cons_list_init(empty);
        return empty;
    }
    return parse_sexpr(tokens, arena);
}


/* ------------- Debugging ------------- */


/**
 * @brief Prints a single, complete S-expression to the given stream using an
 *        iterative approach to avoid stack overflow on deeply nested lists.
 *
 * This function handles the different node types, printing atoms directly
 * and managing lists with an explicit stack.
 *
 * @param stream The file stream to print to (e.g., stdout, stderr).
 * @param expr   A pointer to the root Cons cell of the S-expression to print.
 */
static void print_expression(FILE* stream, const Cons* expr) {
    if (!stream) {
        return;
    }
    if (!expr) {
        fprintf(stream, "nil\n");
        return;
    }

    const Cons* RPAREN_MARKER = (const Cons*)1;
    const Cons* SPACE_MARKER  = (const Cons*)2;

    Vec* worklist = vec_new(sizeof(const Cons*), 256);
    if (!worklist) {
        fprintf(stderr, "Fatal: Could not allocate memory for AST printing.\n");
        return;
    }

    vec_push(&worklist, &expr);

    while (vec_len(worklist) > 0) {
        const Cons* node;
        vec_pop_get(worklist, &node);

        if (node == RPAREN_MARKER) {
            fprintf(stream, ")");
            continue;
        }
        if (node == SPACE_MARKER) {
            fprintf(stream, " ");
            continue;
        }
        if (!node) {
            fprintf(stream, "nil");
            continue;
        }

        switch (node->type) {
            case NODE_ATOM_INT: {
                fprintf(stream, "%lld", *(long long*)(node->car));
                break;
            }

            case NODE_ATOM_SYM: {
                fprintf(stream, "%s", *(char**)(node->car));
                break;
            }

            case NODE_ATOM_STR: {
                fprintf(stream, "\"%s\"", *(char**)(node->car));
                break;
            }

            case NODE_QUOTE:
            case NODE_QUASIQUOTE:
            case NODE_UNQUOTE: {
                char prefix = (node->type == NODE_QUOTE) ? '\''
                            : (node->type == NODE_QUASIQUOTE) ? '`'
                            : ',';
                fprintf(stream, "%c", prefix);

                const Cons* inner_expr = *(const Cons**)(node->car);

                vec_push(&worklist, &inner_expr);
                break;
            }

            case NODE_ATOM_UNINTERNED:{
                fprintf(stream, "%s", *(char**)(node->car));
                break;
            }

            case NODE_NIL: {
                fprintf(stream, "nil");
                break;
            }

            case NODE_OPENING_SEPARATOR:
            case NODE_CLOSING_SEPARATOR: {
                fprintf(stream, "<PARSER_ARTIFACT>");
                break;
            }

            case NODE_LIST: {
                fprintf(stream, "(");
                vec_push(&worklist, &RPAREN_MARKER);

                const ConsList* sublist = *(const ConsList**)(node->car);
                if (sublist && sublist->head) {
                    Vec* children = vec_new(sizeof(const Cons*), sublist->length);
                    if (!children) {
                        fprintf(stderr, "Fatal: OOM during AST printing.\n");
                        vec_free(&worklist);
                        return;
                    }

                    for (const Cons* current = sublist->head; current != NULL; current = current->cdr) {
                        vec_push(&children, &current);
                    }

                    bool first_child = true;
                    while (vec_len(children) > 0) {
                        const Cons* child_node;
                        vec_pop_get(children, &child_node);

                        if (!first_child) {
                            vec_push(&worklist, &SPACE_MARKER);
                        }
                        vec_push(&worklist, &child_node);
                        first_child = false;
                    }
                    vec_free(&children);
                }
                break;
            }

            default: {
                fprintf(stream, "<UNKNOWN_NODE_TYPE>");
                break;
            }
        }
    }

    vec_free(&worklist);
    fprintf(stream, "\n");
}

void print_program(ConsList* program) {
    if (!program) {
        fprintf(stdout, "(empty program)\n");
        return;
    }
    for (const Cons* expr = program->head; expr != NULL; expr = expr->cdr) {
        print_expression(stdout, expr);
    }
}
