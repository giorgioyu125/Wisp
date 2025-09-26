/*
 * @file lexer.h
*/

#include <stddef.h>

#include "vec.h"

typedef enum {
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_INTEGER,
	TOKEN_FLOAT,
	TOKEN_STRING, 
	TOKEN_IDENTIFIER,
	TOKEN_QUOTE,
	TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;

	size_t value_len;
    char* value;
} Token;

/* ----------------------- LEXER --------------------- */ 

Vec* lex_tokens(const char* source, size_t source_len);
