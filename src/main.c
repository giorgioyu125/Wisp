/*
 * @file main.c
 * @brief Entry point of Wisp.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include "lexer.h"
#include "readfile.h"
#include "vec.h"

int
main (int argc, char **argv)
{
    clock_t start = clock ();

    if (argc != 2)
      {
	  fprintf (stderr, "Usage: %s <file>\n", argv[0]);
	  return -1;
      }

    size_t len = 0;
    char *data = read_file (argv[1], &len);
    if (!data)
      {
	  return -1;
      }

    Vec *tokens = lex_tokens (data, len);
    if (!tokens)
      {
	  fprintf (stderr, "Failed to lex tokens\n");
	  free (data);
	  return -1;
      }

    print_token_vec (tokens);

    int err = vec_free (&tokens);
    free (data);

    if (err != 0)
      {
	  fprintf (stderr, "vec_free: failed to free tokens vector\n");
	  return -1;
      }

    clock_t end = clock ();
    double total_time = (double) (end - start) / CLOCKS_PER_SEC;

    fprintf (stderr, "Total time: %.6f\n", total_time);
    return 0;
}
