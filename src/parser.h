/*
* @file parser.h
* @brief A expression annotator based parser to determine the id of the tokens and turn them 
*        into a pair of virtual stacks of Operation/Values
*/

#ifndef PARSER_H
#define PARSER_H

#include "vec.h"

/* ----------------- Annotator --------------- */

/*
 * @brief This functions updates the s_exprid field of every 
 *        accordingly and removes parenthesis.
 * @param[out] Tokens is the return value of the lexer
 *        and the out param of the annotator.
 * @retval This signal if an error occured while parsing the Tokens.
 */
int annotate_tokens(Vec * Tokens);

/* -------------- Syntax Checker ------------- */



#endif
