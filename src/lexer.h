/**
 * @file lexer.h
 * @brief Lexical analyzer for a simple Lisp-like language.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include "vec.h"

/**
 * @enum TokenType
 * @brief Enumeration of possible token types.
 *
 * Each token in the input source is classified into one of these types.
 */
typedef enum {
    TOKEN_LPAREN,    ///< Left parenthesis '('
    TOKEN_RPAREN,    ///< Right parenthesis ')'
    TOKEN_INTEGER,   ///< Integer literal (e.g., '42')
    TOKEN_FLOAT,     ///< Floating-point literal (e.g., '3.14')
    TOKEN_STRING,    ///< String literal (e.g., '"hello"')
    TOKEN_IDENTIFIER,///< Identifier (e.g., 'foo', 'bar')
    TOKEN_QUOTE,     ///< Quote character '\''
    TOKEN_COMMA,     ///< Comma ','
    TOKEN_ERROR      ///< Invalid or unrecognized token
} TokenType;

/**
 * @struct Token
 * @brief Represents a token in the input source.
 *
 * Each token has a type and an optional value (e.g., the actual string for identifiers or literals).
 */
typedef struct {
    TokenType type;  ///< Type of the token
    size_t value_len;///< Length of the token's value (if applicable)
    char* value;     ///< Pointer to the token's value (points into the original source)
} Token;


/* ------------------------- LEXER ---------------------- */


/**
 * @brief Lexes the input source code into a vector of tokens.
 *
 * This function processes the input string, identifies tokens, and stores them
 * in a dynamically allocated vector. The caller is responsible for freeing the
 * returned vector using `vec_free`.
 *
 * @param source Pointer to the input source code.
 * @param source_len Length of the input source code.
 * @return Vec* A vector of tokens, or NULL if memory allocation fails.
 */
Vec* lex_tokens(const char* source, size_t source_len);


/**
 * @brief Retrieves a token from a vector of tokens by index.
 *
 * @param v_ptr Pointer to the vector of tokens.
 * @param idx Index of the token to retrieve.
 * @return Token The token at the specified index.
 */
Token vec_get_token(const Vec* v_ptr, size_t idx);


/* ------------------- DEBUGGING --------------------- */ 


/**
 * @brief Print a single token's value and type to stderr.
 * @param token The token to print.
 */
void print_token(Token token);

/**
 * @brief Print all tokens in a vector to stderr.
 * @param tokens Pointer to the token vector. If NULL, prints error message.
 */
void print_token_vec(Vec* tokens);


#endif // LEXER_H
