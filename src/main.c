/*
 * @file main.c
 * @brief Entry point of Wisp.
 */

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "lexer.h"
#include "readfile.h"
#include "src/arena.h"
#include "vec.h"
#include "parser.h"
#include "symtab.h"

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

    size_t initial_arena_size = (file->size > 1024*1024) ? file->size * 2 : 2 * 1024 * 1024;
    Arena* global_arena = arena_create(initial_arena_size);

    printf("=== Lexing ===\n");
    Vec *tokens = lex_tokens(file->data, file->size, &global_arena);
    if (!tokens) {
        fprintf(stderr, "lex_tokens: failed to lex tokens\n");
        filebuffer_free(file);
        return -1;
    }
    printf("Lexed %zu tokens", vec_len(tokens));

    if (!global_arena) {
        fprintf(stderr, "Failed to create global memory arena.\n");
        return -1;
    }
    ConsList* program_ast = parse_program(tokens, &global_arena);
    if (program_ast) {
        printf("\n=== Parsed AST ===\n");
        print_program(program_ast);
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }

    printf("=== Cleanup ===\n");
    filebuffer_free(file);
    arena_free(global_arena);

    clock_t end = clock();
    double total_time = (double) (end - start) / CLOCKS_PER_SEC;

    fprintf(stderr, "Total time: %.6f seconds\n", total_time);
    return 0;
}

