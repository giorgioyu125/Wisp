/*
 * @file main.c
 * @brief Entry point of Wisp.
 */

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "lexer.h"
#include "readfile.h"
#include "vec.h"
#include "parser.h"
#include "svm.h"
#include "symtab.h"

SymTab* g_symtab = { 0 };

/* --------------------- Main Function --------------------- */

int main(int argc, char **argv)
{
    clock_t start = clock();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return -1;
    }

    FileBuffer *file = read_file(argv[1]);
    if (!file) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", argv[1]);
        return -1;
    }

    printf("=== Lexing ===\n");
    Vec *tokens = lex_tokens(file->data, file->size);
    if (!tokens) {
        fprintf(stderr, "lex_tokens: failed to lex tokens\n");
        filebuffer_free(file);
        return -1;
    }
    printf("Lexed %zu tokens\n", vec_len(tokens));

    printf("=== Annotating ===\n");
    int err = annotate_tokens(tokens);
    if (err != 0) {
        fprintf(stderr, "annotate_tokens [%d]: failed to annotate tokens vector\n", err);
        filebuffer_free(file);
        vec_free(&tokens);
        return -1;
    }
    printf("Annotation successful\n");

    printf("=== Parsing ===\n");
    ProgramFlux* program = parse(tokens);
    if (!program){
        fprintf(stderr, "parse: failed to construct the ProgramFlux\n");
        filebuffer_free(file);
        vec_free(&tokens);
        return -1;
    }
    printf("Parsed %zu S-expressions\n", vec_len(program->sexprs));

    printf("\n=== Program Structure ===\n");
    for (size_t expr_idx = 0; expr_idx < vec_len(program->sexprs); expr_idx++) {
        SExpr* sexpr = *(SExpr**)vec_at(program->sexprs, expr_idx);

        printf("[ID:%zu] ", sexpr->id);
        for (size_t i = sexpr->start_idx; i <= sexpr->end_idx; i++) {
            Token tok = vec_get_token(program->tokens, i);
            if (tok.type != TOKEN_IGNORE) {
                printf("%.*s ", (int)tok.value_len, tok.value);
            }
        }
        printf("\n");
    }

    printf("\n=== Starting Evaluation ===\n");
    int eval_result = evaluate_program(program);
    if (eval_result != 0) {
        fprintf(stderr, "Evaluation failed with code %d\n", eval_result);
    }

    printf("=== Cleanup ===\n");
    err = vec_free(&tokens);
    filebuffer_free(file);
    program_flux_free(program);
    if (err != 0) {
        fprintf(stderr, "vec_free: failed to free tokens vector\n");
        return -1;
    }

    clock_t end = clock();
    double total_time = (double) (end - start) / CLOCKS_PER_SEC;

    fprintf(stderr, "Total time: %.6f seconds\n", total_time);
    return 0;
}

