/*
* @file parser.c 
* @brief A expression annotator based parser to determine the id of the tokens and turn them 
*        into a pair of virtual stacks of Operation/Values
*/
#include "vec.h"
#include "lexer.h"

#include <string.h>


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
