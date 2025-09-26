/*
 * @file lexer.h
*/

#include "vec.h"
#include "lexer.h"

#include <ctype.h>
#include <string.h>

Vec* lex_tokens(const char* source, size_t source_len) {
    Vec* tokens = vec_new(64, sizeof(Token));
    if (!tokens) return NULL;

    const char* ptr = source;
    const char* end = source + source_len;

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
                .type = TOKEN_LPAREN,
                .value = (char*)ptr,
                .value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }

        // RPAREN
        if (c == ')') {
            Token tok = {
                .type = TOKEN_RPAREN,
                .value = (char*)ptr,
                .value_len = 1
            };
            vec_push(&tokens, &tok);
            ptr++;
            continue;
        }

		// QUOTATION MARK
		if (c == '\'') {
		    Token tok = {
		        .type = TOKEN_QUOTE,
		        .value = (char*)ptr,
		        .value_len = 1
		    };
		    vec_push(&tokens, &tok);
		    ptr++;
		    continue;
		}

        // STRING
		if (c == '"') {
		    const char* start = ptr++;
		    while (ptr < end && *ptr != '"') {
		        if (*ptr == '\\' && (ptr + 1 < end)) ptr++; // skip escape
		        ptr++;
		    }
		
		    if (ptr < end && *ptr == '"') {
		        Token tok = {
		            .type = TOKEN_STRING,
		            .value = (char*)start,
		            .value_len = (size_t)(ptr - start + 1)
		        };
		        vec_push(&tokens, &tok);
		        ptr++;
		    } else {
		        Token tok = {
		            .type = TOKEN_ERROR,
		            .value = (char*)start,
		            .value_len = (size_t)(ptr - start)
		        };
		        vec_push(&tokens, &tok);
		    }
		
		    continue;
		}

        // INTEGER or IDENTIFIER
		if (isdigit(c) || ((c == '-' || c == '+') && ptr + 1 < end && (isdigit(ptr[1]) || ptr[1] == '.'))) {
		    const char* start = ptr;
		    bool is_float = false;
			
		    if (c == '-' || c == '+') {
		        ptr++;
		    }
			
		    while (ptr < end && isdigit(*ptr)) ptr++;
			
		    if (ptr < end && *ptr == '.') {
		        is_float = true;
		        ptr++;
				
		        while (ptr < end && isdigit(*ptr)) ptr++;
		    }
			
		    if (ptr < end && (*ptr == 'e' || *ptr == 'E')) {
		        is_float = true;
		        ptr++;
				
		        if (ptr < end && (*ptr == '+' || *ptr == '-')) {
		            ptr++;
		        }
				
		        if (ptr < end && isdigit(*ptr)) {
		            while (ptr < end && isdigit(*ptr)) ptr++;
		        } else {
					ptr = start;
    				goto identifier;
		        }
		    }
			
		    Token tok = {
		        .type = is_float ? TOKEN_FLOAT : TOKEN_INTEGER,
		        .value = (char*)start,
		        .value_len = (size_t)(ptr - start),
		    };
		    vec_push(&tokens, &tok);
		    continue;
		}

identifier:
		c = *ptr;
        // IDENTIFIER: starts with letter or symbol
        if (isalpha(c) || strchr("!@#$%^&*-+=<>/?", c)) {
            const char* start = ptr;
            while (ptr < end && (isalnum(*ptr) || strchr("!@#$%^&*-+=<>/?", *ptr))) {
                ptr++;
            }
            Token tok = {
                .type = TOKEN_IDENTIFIER,
                .value = (char*)start,
                .value_len = (size_t)(ptr - start)
            };
            vec_push(&tokens, &tok);
            continue;
        }

		Token tok = {
		    .type = TOKEN_ERROR,
		    .value = (char*)ptr,
		    .value_len = 1
		};
		vec_push(&tokens, &tok);
		ptr++;
    }

    return tokens;
}
