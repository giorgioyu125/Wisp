/*
 * @file lexer.h
 */
#include "lexer.h"
#include "vec.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

Vec *lex_tokens(const char *source, size_t source_len)
{
    Vec *tokens = vec_new(64, sizeof(Token));
    if (!tokens)
        return NULL;

    const char *ptr = source;
    const char *end = source + source_len;

    while (ptr < end) {
        char c = *ptr;

        // Skip whitespace
        if (isspace(c)) {
            ptr++;
            continue;
        }
        // LPAREN
        if (c == '(') {
            Token tok = {
                .type = TOKEN_LPAREN,.value =
                    (char *) ptr,.value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }
        // RPAREN
        if (c == ')') {
            Token tok = {
                .type = TOKEN_RPAREN,.value =
                    (char *) ptr,.value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }
        // Skip line comments (; until newline)
        if (c == ';') {
            while (++ptr < end && *ptr != '\n');
            continue;
        }
        // QUOTE OPERATOR
        if (c == '\'') {
            Token tok = {
                .type = TOKEN_QUOTE,.value =
                    (char *) ptr,.value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }
        // COMMA OPERATOR
        if (c == ',') {
            Token tok = {
                .type = TOKEN_COMMA,.value =
                    (char *) ptr,.value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }
        // STRING
        if (c == '"') {
            const char *start = ptr++;
            while (ptr < end && *ptr != '"') {
                if (*ptr == '\\' && (ptr + 1 < end))
                    ptr++;      // skip escape
                ptr++;
            }

            if (ptr < end && *ptr == '"') {
                Token tok = {
                    .type = TOKEN_STRING,
                    .value = (char *) start,
                    .value_len = (size_t) (ptr - start + 1)
                };
                vec_push(&tokens, &tok);
                ptr++;
            } else {
                Token tok = {
                    .type = TOKEN_ERROR,
                    .value = (char *) start,
                    .value_len = (size_t) (ptr - start)
                };
                vec_push(&tokens, &tok);
            }

            continue;
        }
        // INTEGER
        if (isdigit(c) || ((c == '-' || c == '+') && ptr + 1 < end &&
                           (isdigit(ptr[1]) || ptr[1] == '.'))) {
            const char *start = ptr;
            bool is_float = false;

            if (c == '-' || c == '+') {
                ptr++;
            }

            while (ptr < end && isdigit(*ptr))
                ptr++;

            if (ptr < end && *ptr == '.') {
                is_float = true;
                ptr++;

                while (ptr < end && isdigit(*ptr))
                    ptr++;
            }

            if (ptr < end && (*ptr == 'e' || *ptr == 'E')) {
                is_float = true;
                ptr++;

                if (ptr < end && (*ptr == '+' || *ptr == '-')) {
                    ptr++;
                }

                if (ptr < end && isdigit(*ptr)) {
                    while (ptr < end && isdigit(*ptr))
                        ptr++;
                } else {
                    ptr = start;
                    goto identifier;
                }
            }

            Token tok = {
                .type = is_float ? TOKEN_FLOAT : TOKEN_INTEGER,
                .value = (char *) start,
                .value_len = (size_t) (ptr - start),
            };
            vec_push(&tokens, &tok);
            continue;
        }

        identifier:
        c = *ptr;
        // IDENTIFIER
        if (isalpha(c) || strchr("!@#$%^&*-+=<>/?", c)) {
            const char *start = ptr;
            while (ptr < end && (isalnum(*ptr)
                                 || strchr("!@#$%^&*-+=<>/?", *ptr))) {
                ptr++;
            }
            Token tok = {
                .type = TOKEN_IDENTIFIER,
                .value = (char *) start,
                .value_len = (size_t) (ptr - start)
            };
            vec_push(&tokens, &tok);
            continue;
        }

        Token tok = {
            .type = TOKEN_ERROR,
            .value = (char *) ptr,
            .value_len = 1
        };
        vec_push(&tokens, &tok);
        ptr++;
    }

    return tokens;
}

Token vec_get_token(const Vec *v_ptr, size_t idx)
{
    if (!v_ptr || idx >= v_ptr->elem_num) {
        Token error_token = { 0 };
        return error_token;
    }
    return *(Token *) ((char *) (v_ptr + 1) + (idx * v_ptr->elem_size));
}

/* ----------------------- DEBUGGING ------------------- */

void print_token_id(size_t id)
{
    fprintf(stderr, "%zu", id);
}

void print_type(TokenType type)
{
    switch (type) {
    case TOKEN_LPAREN:
        {
            fprintf(stderr, "TOKEN_LPAREN");
            break;
        }

    case TOKEN_RPAREN:
        {
            fprintf(stderr, "TOKEN_RPAREN");
            break;
        }

    case TOKEN_INTEGER:
        {
            fprintf(stderr, "TOKEN_INTEGER");
            break;
        }

    case TOKEN_FLOAT:
        {
            fprintf(stderr, "TOKEN_FLOAT");
            break;
        }

    case TOKEN_STRING:
        {
            fprintf(stderr, "TOKEN_STRING");
            break;
        }

    case TOKEN_IDENTIFIER:
        {
            fprintf(stderr, "TOKEN_IDENTIFIER");
            break;
        }

    case TOKEN_QUOTE:
        {
            fprintf(stderr, "TOKEN_QUOTE");
            break;
        }

    case TOKEN_COMMA:
        {
            fprintf(stderr, "TOKEN_COMMA");
            break;
        }


    case TOKEN_IGNORE:
        {
            fprintf(stderr, "TOKEN_IGNORE");
            break;
        }

    case TOKEN_ERROR:
        {
            fprintf(stderr, "TOKEN_ERROR");
            break;
        }
    }


}

void print_token_value(char *value, size_t value_len)
{
    fwrite(value, 1, value_len, stderr);
}

void print_token(Token token)
{
    if (token.type == TOKEN_IGNORE)
        return;
    print_token_value(token.value, token.value_len);
    fprintf(stderr, " ");
    print_type(token.type);
    fprintf(stderr, " ");
    print_token_id(token.s_exprid);
    fprintf(stderr, "\n");
}

void print_token_vec(Vec *tokens)
{
    if (!tokens) {
        fprintf(stderr, "Error: NULL token vector\n");
        return;
    }

    for (size_t i = 0; i < vec_len(tokens); i++) {
        Token token = vec_get_token(tokens, i);
        print_token(token);
    }
}
