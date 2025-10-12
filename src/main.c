/*
 * @file main.c
 * @brief Entry point of Wisp.
 */

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "lexer.h"
#include "readfile.h"
#include "vec.h"
#include "parser.h"

int main(int argc, char **argv)
{
    clock_t start = clock();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return -1;
    }

    FileBuffer *file = read_file(argv[1]);
    if (!file) {
        return -1;
    }

    Vec *tokens = lex_tokens(file->data, file->size);
    if (!tokens) {
        fprintf(stderr, "lex_tokens: failed to lex tokens\n");
        filebuffer_free(file);
        return -1;
    }

    int err = annotate_tokens(tokens);
    if (err != 0) {
        fprintf(stderr,
                "annotate_tokens [%d]: failed to annotate tokens vector\n",
                err);
        filebuffer_free(file);
        vec_free(&tokens);
        return -1;
    }

    ProgramFlux* program = parse(tokens);
    if (!program){
        fprintf(stderr, "parse [%d]: failed to construct the ProgramFlux\n", err);
        filebuffer_free(file);
        vec_free(&tokens);
        return -1;
    }

    err = vec_free(&tokens);
    filebuffer_free(file);
    free(program);
    if (err != 0) {
        fprintf(stderr, "vec_free: failed to free tokens vector\n");
        return -1;
    }

    clock_t end = clock();
    double total_time = (double) (end - start) / CLOCKS_PER_SEC;

    fprintf(stderr, "Total time: %.6f\n", total_time);
    return 0;
}
