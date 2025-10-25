/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -t builtins.gperf  */
/* Computed positions: -k'1-2' */

/*
 * @file find_builtin.c
 */
#include "builtins.h"

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "builtins.gperf"

#include "eval.h"
#line 4 "builtins.gperf"
struct BuiltinName { const char* name; BuiltinType type; };

#define TOTAL_KEYWORDS 29
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 32
/* maximum key range = 32, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 23, 30, 33, 25, 33, 20, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      10, 15,  0, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 10,  5,  5,
      20,  0, 20, 33, 33, 33,  5, 33,  0, 10,
       0, 33,  5, 15,  0,  5, 33, 10,  0, 15,
      33,  0,  5, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[1]+1];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct BuiltinName *
find_builtin (register const char *str, register size_t len)
{
  static struct BuiltinName wordlist[] =
    {
      {""},
#line 13 "builtins.gperf"
      {">", BUILTIN_GREATER_THAN},
#line 15 "builtins.gperf"
      {">=", BUILTIN_GREATER_EQ},
#line 28 "builtins.gperf"
      {"eq?", BUILTIN_EQ},
#line 34 "builtins.gperf"
      {"exit", BUILTIN_EXIT},
#line 23 "builtins.gperf"
      {"null?", BUILTIN_IS_NULL},
#line 29 "builtins.gperf"
      {"equal?", BUILTIN_EQUAL},
#line 24 "builtins.gperf"
      {"number?", BUILTIN_IS_NUMBER},
#line 18 "builtins.gperf"
      {"cdr", BUILTIN_CDR},
#line 19 "builtins.gperf"
      {"list", BUILTIN_LIST},
#line 22 "builtins.gperf"
      {"list?", BUILTIN_IS_LIST},
#line 12 "builtins.gperf"
      {"<", BUILTIN_LESS_THAN},
#line 14 "builtins.gperf"
      {"<=", BUILTIN_LESS_EQ},
#line 17 "builtins.gperf"
      {"car", BUILTIN_CAR},
#line 16 "builtins.gperf"
      {"cons", BUILTIN_CONS},
#line 21 "builtins.gperf"
      {"pair?", BUILTIN_IS_PAIR},
#line 11 "builtins.gperf"
      {"=", BUILTIN_NUM_EQ},
#line 26 "builtins.gperf"
      {"symbol?", BUILTIN_IS_SYMBOL},
#line 10 "builtins.gperf"
      {"mod", BUILTIN_MOD},
#line 33 "builtins.gperf"
      {"eval", BUILTIN_EVAL},
#line 27 "builtins.gperf"
      {"procedure?", BUILTIN_IS_PROCEDURE},
#line 9 "builtins.gperf"
      {"/", BUILTIN_DIV},
#line 25 "builtins.gperf"
      {"string?", BUILTIN_IS_STRING},
      {""},
#line 8 "builtins.gperf"
      {"*", BUILTIN_MUL},
#line 20 "builtins.gperf"
      {"atom?", BUILTIN_IS_ATOM},
#line 7 "builtins.gperf"
      {"-", BUILTIN_SUB},
#line 31 "builtins.gperf"
      {"newline", BUILTIN_NEWLINE},
      {""}, {""},
#line 32 "builtins.gperf"
      {"apply", BUILTIN_APPLY},
#line 6 "builtins.gperf"
      {"+", BUILTIN_ADD},
#line 30 "builtins.gperf"
      {"display", BUILTIN_DISPLAY}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 35 "builtins.gperf"

