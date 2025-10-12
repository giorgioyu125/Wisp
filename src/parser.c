/*
* @file parser.c 
* @brief A expression annotator based parser to determine the id of the tokens and turn them 
*        into a pair of virtual stacks of Operation/Values
*/
#include "src/parser.h"
#include "vec.h"
#include "lexer.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <stdint.h>

/* ----------------- Annotator --------------- */

int annotate_tokens(Vec *Tokens)
{
    if (!Tokens)
        return -1;
    if (Tokens->elem_num <= 0)
        return -2;

    Vec *id_stack = vec_new(sizeof(size_t), 256);
    if (!id_stack)
        return -3;

    size_t current_id = 0;
    Token *current_token;

    for (size_t i = 0; i < Tokens->elem_num; i++) {
        current_token = (Token *) vec_at(Tokens, i);

        if (current_token->type == TOKEN_LPAREN) {
            current_id++;
            current_token->s_exprid = current_id;
            vec_push(&id_stack, &current_id);
            current_token->type = TOKEN_IGNORE;
        } else if (current_token->type == TOKEN_RPAREN) {
            if (id_stack->elem_num > 0) {
                current_token->s_exprid = *(size_t *) vec_peek(id_stack);
                vec_pop_discard(id_stack);
            } else {
                vec_free(&id_stack);
                return -4;
            }
            current_token->type = TOKEN_IGNORE;
        } else {
            if (id_stack->elem_num > 0) {
                current_token->s_exprid = *(size_t *) vec_peek(id_stack);
            } else {
                current_token->s_exprid = 0;
            }
        }
    }

    int result = (id_stack->elem_num == 0) ? 0 : -5;
    vec_free(&id_stack);
    return result;
}

ProgramFlux* parse(Vec* annotated_tokens) {
    if (!annotated_tokens || annotated_tokens->elem_num == 0) {
        return NULL;
    }

    size_t max_id = 0;
    size_t expr_count = 0;
    for (size_t i = 0; i < annotated_tokens->elem_num; i++) {
        Token* tok = (Token*)vec_at(annotated_tokens, i);
        if (tok->type != TOKEN_IGNORE) {
            if (tok->s_exprid > max_id) {
                max_id = tok->s_exprid;
            }
        }
    }

    typedef struct {
        size_t start;
        size_t end;
        bool found;
    } Bounds;

    Bounds* bounds = calloc(max_id + 1, sizeof(Bounds));
    if (!bounds) return NULL;

    for (size_t i = 0; i <= max_id; i++) {
        bounds[i].start = SIZE_MAX;
        bounds[i].end = 0;
        bounds[i].found = false;
    }

    for (size_t i = 0; i < annotated_tokens->elem_num; i++) {
        Token* tok = (Token*)vec_at(annotated_tokens, i);

        if (tok->type != TOKEN_IGNORE) {
            size_t id = tok->s_exprid;

            if (!bounds[id].found) {
                bounds[id].start = i;
                bounds[id].found = true;
                expr_count++;
            }
            bounds[id].end = i;
        }
    }

    ProgramFlux* flux = malloc(sizeof(ProgramFlux));
    if (!flux) {
        free(bounds);
        return NULL;
    }

    flux->sexprs = vec_new(sizeof(SExpr*), expr_count > 0 ? expr_count : 1);
    if (!flux->sexprs) {
        free(flux);
        free(bounds);
        return NULL;
    }

    flux->tokens = annotated_tokens;
    flux->max_depth = max_id;

    for (size_t id = max_id + 1; id > 0; id--) {
        size_t actual_id = id - 1;

        if (!bounds[actual_id].found) {
            continue;
        }

        SExpr* sexpr = malloc(sizeof(SExpr));
        if (!sexpr) {
            for (size_t j = 0; j < vec_len(flux->sexprs); j++) {
                SExpr** stored = (SExpr**)vec_at(flux->sexprs, j);
                if (stored && *stored) free(*stored);
            }
            vec_free(&flux->sexprs);
            free(flux);
            free(bounds);
            return NULL;
        }
        sexpr->id = actual_id;
        sexpr->start_idx = bounds[actual_id].start;
        sexpr->end_idx = bounds[actual_id].end;
        sexpr->tokens = annotated_tokens;

        if (vec_push(&flux->sexprs, &sexpr) != 0) {
            free(sexpr);
            for (size_t j = 0; j < vec_len(flux->sexprs); j++) {
                SExpr** stored = (SExpr**)vec_at(flux->sexprs, j);
                if (stored && *stored) free(*stored);
            }
            vec_free(&flux->sexprs);
            free(flux);
            free(bounds);
            return NULL;
        }
    }

    free(bounds);
    return flux;
}
