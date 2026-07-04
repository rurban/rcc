/* ANSI-C code produced by gperf version 3.2 */
/* Command-line: gperf -m 10 --output-file=src/keywords.h src/keywords.gperf  */
/* Computed positions: -k'1,3,8,11-12,17,19,$' */

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

#line 1 "src/keywords.gperf"

#include "keyword_ids.h"
#include <limits.h>
#line 6 "src/keywords.gperf"
struct keyword_entry { const char *name; int id; unsigned flags; char *interned; };

#define TOTAL_KEYWORDS 254
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 28
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 486
/* maximum key range = 485, duplicates = 0 */

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
  static const unsigned short asso_values[] =
    {
      487, 487, 487, 487, 487, 487, 487, 487, 487, 487,
      487, 487, 487, 487, 487, 487, 487, 487, 487, 487,
      487, 487, 487, 487, 487, 487, 487, 487, 487, 487,
      487, 487, 487, 487, 487, 487, 487, 487, 487, 487,
      487, 487, 487, 487, 487, 487, 487, 487, 487, 487,
       83, 487,  76, 487,   0, 487,  16, 487, 487, 487,
      487, 487, 487, 487, 487, 487, 487, 487,   4, 487,
      487,   0,   3,   0, 487, 487,   0,   0,   0, 487,
      487,   1, 487,   0, 487,   0, 487, 487, 487,   0,
      487, 487, 487, 487, 487,   5,   3,  71,  34,   0,
       50,  32,   2, 186,  99,   5,  28,  62,   0, 150,
      131,  35, 104, 203,  50,   6,  16, 121, 170, 149,
       21,  43, 130,   0, 487, 487, 487, 487, 487
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[18]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 18:
      case 17:
        hval += asso_values[(unsigned char)str[16]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 16:
      case 15:
      case 14:
      case 13:
      case 12:
        hval += asso_values[(unsigned char)str[11]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 11:
        hval += asso_values[(unsigned char)str[10]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 10:
      case 9:
      case 8:
        hval += asso_values[(unsigned char)str[7]];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
        hval += asso_values[(unsigned char)str[2]+1];
#if (defined __cplusplus && (__cplusplus >= 201703L || (__cplusplus >= 201103L && defined __clang__ && __clang_major__ + (__clang_minor__ >= 9) > 3))) || (__STDC_VERSION__ >= 202000L && ((defined __GNUC__ && __GNUC__ >= 10) || (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || (defined __clang__ && __clang_major__ >= 10)
      __attribute__ ((__fallthrough__));
#endif
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

static const unsigned char lengthtable[] =
  {
     0,  0,  2,  2,  4,  2,  2,  0,  0,  2,  4,  1,  0,  0,
     6,  8,  0,  7,  0,  0,  0,  7,  0,  8, 14, 15, 16, 17,
    14, 15,  9, 15, 12, 13,  6, 19,  0, 16, 15, 16, 14, 15,
    10, 12,  5,  5,  6,  5,  6,  0, 11,  7,  4,  0,  0, 18,
     5, 18,  9, 15,  6,  3,  7,  0,  6,  0,  4, 17,  6,  0,
    17,  5, 23,  8,  9,  0,  0,  0,  7,  0, 10,  0,  0,  0,
     4,  9, 10,  2,  4,  7,  0,  0, 11, 10,  7, 11,  3,  0,
    16, 13, 10, 14, 18,  5,  0,  8,  8,  8, 11,  0,  5,  0,
     0,  7,  5,  0,  7,  0,  0,  0,  0,  4,  6,  0,  0,  0,
     0,  5, 13,  8, 11,  5,  0,  7, 13, 14, 14,  0,  8,  6,
     6, 17, 10, 10,  0,  3, 11,  0, 11,  6,  0, 10, 16, 13,
     8,  0,  7,  0, 21,  8,  5,  6,  0,  0,  0, 10,  0,  8,
    16, 13, 21,  0,  0,  0,  9,  8, 15, 24,  9, 16,  0,  0,
     0,  5,  0,  7, 12,  0,  7,  0, 18,  0,  0,  6,  5,  0,
     0, 11,  9, 14,  0, 10, 17, 18, 17, 18, 21, 10, 13,  8,
     6,  0, 23,  0,  5,  6, 12,  0,  4, 17,  4,  8, 13,  4,
    16,  4,  0,  6,  7,  0,  0,  4, 20, 17,  8,  4,  0, 16,
     0, 16,  0, 18, 19, 15, 16,  8, 19, 20,  0,  4, 19, 16,
     4,  0, 19,  0, 18,  8,  6,  7, 18,  9, 14,  9, 24,  0,
     0,  0,  9, 21, 17,  0, 10, 10,  6,  0,  0, 19,  0,  0,
     0, 18,  0,  0,  0,  5, 18,  0,  0,  0,  6, 15,  0,  7,
     0, 28,  0,  0,  6, 19, 20,  0, 20,  6,  0, 20, 18, 25,
     6, 17,  0, 18, 16,  0, 18,  8,  0, 16,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0, 20,  8,  6,  0,  0,  0,  0, 18,
    22, 18,  7,  0,  0,  0, 22,  0, 13,  0,  4,  0,  0,  0,
     0, 28,  0,  0, 17,  3,  4, 13,  0,  0,  0,  0,  0,  0,
     0,  0, 14, 18,  0,  0,  0,  0,  0, 19, 21,  0,  0,  0,
     0,  0, 18, 13, 27, 10,  0,  8,  0,  0,  0,  0,  0,  6,
     8,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0,  0,  0,  0,
     0,  0, 27,  0, 21,  0,  0,  0,  3,  0, 21,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0, 24,  0, 12,  0,  0,
     0,  0,  0,  0,  0, 16,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0, 19,  0,
     0,  0,  0,  0,  0,  0,  0, 25,  0,  0, 22
  };

#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const struct keyword_entry keyword_table[] =
  {
    {(char*)0}, {(char*)0},
#line 109 "src/keywords.gperf"
    {"SI", ID_SI, KW_MODE, NULL},
#line 107 "src/keywords.gperf"
    {"QI", ID_QI, KW_MODE, NULL},
#line 128 "src/keywords.gperf"
    {"NULL", ID_NULL, KW_SPECIAL, NULL},
#line 108 "src/keywords.gperf"
    {"HI", ID_HI, KW_MODE, NULL},
#line 110 "src/keywords.gperf"
    {"DI", ID_DI, KW_MODE, NULL},
    {(char*)0}, {(char*)0},
#line 15 "src/keywords.gperf"
    {"if", ID_IF, KW_CONTROL, NULL},
#line 147 "src/keywords.gperf"
    {"labs", ID_LABS, KW_BUILTIN, NULL},
#line 127 "src/keywords.gperf"
    {"_", ID__, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0},
#line 117 "src/keywords.gperf"
    {"sizeof", ID_SIZEOF, KW_SPECIAL, NULL},
#line 115 "src/keywords.gperf"
    {"_Generic", ID__GENERIC, KW_SPECIAL, NULL},
    {(char*)0},
#line 151 "src/keywords.gperf"
    {"fprintf", ID_FPRINTF, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 156 "src/keywords.gperf"
    {"sprintf", ID_SPRINTF, KW_BUILTIN, NULL},
    {(char*)0},
#line 129 "src/keywords.gperf"
    {"__retbuf", ID___RETBUF, KW_SPECIAL, NULL},
#line 180 "src/keywords.gperf"
    {"__builtin_clzl", ID___BUILTIN_CLZL, KW_BUILTIN, NULL},
#line 181 "src/keywords.gperf"
    {"__builtin_clzll", ID___BUILTIN_CLZLL, KW_BUILTIN, NULL},
#line 177 "src/keywords.gperf"
    {"__builtin_clrsbl", ID___BUILTIN_CLRSBL, KW_BUILTIN, NULL},
#line 178 "src/keywords.gperf"
    {"__builtin_clrsbll", ID___BUILTIN_CLRSBLL, KW_BUILTIN, NULL},
#line 190 "src/keywords.gperf"
    {"__builtin_ffsl", ID___BUILTIN_FFSL, KW_BUILTIN, NULL},
#line 191 "src/keywords.gperf"
    {"__builtin_ffsll", ID___BUILTIN_FFSLL, KW_BUILTIN, NULL},
#line 103 "src/keywords.gperf"
    {"__stdcall", ID___STDCALL, KW_ASM, NULL},
#line 197 "src/keywords.gperf"
    {"__builtin_llabs", ID___BUILTIN_LLABS, KW_BUILTIN, NULL},
#line 118 "src/keywords.gperf"
    {"__FUNCTION__", ID___FUNCTION__, KW_SPECIAL, NULL},
#line 189 "src/keywords.gperf"
    {"__builtin_ffs", ID___BUILTIN_FFS, KW_BUILTIN, NULL},
#line 73 "src/keywords.gperf"
    {"struct", ID_STRUCT, KW_TYPE, NULL},
#line 119 "src/keywords.gperf"
    {"__PRETTY_FUNCTION__", ID___PRETTY_FUNCTION__, KW_SPECIAL, NULL},
    {(char*)0},
#line 195 "src/keywords.gperf"
    {"__builtin_isinfl", ID___BUILTIN_ISINFL, KW_BUILTIN, NULL},
#line 193 "src/keywords.gperf"
    {"__builtin_isinf", ID___BUILTIN_ISINF, KW_BUILTIN, NULL},
#line 194 "src/keywords.gperf"
    {"__builtin_isinff", ID___BUILTIN_ISINFF, KW_BUILTIN, NULL},
#line 187 "src/keywords.gperf"
    {"__builtin_ctzl", ID___BUILTIN_CTZL, KW_BUILTIN, NULL},
#line 188 "src/keywords.gperf"
    {"__builtin_ctzll", ID___BUILTIN_CTZLL, KW_BUILTIN, NULL},
#line 46 "src/keywords.gperf"
    {"__restrict", ID___RESTRICT, KW_QUAL, NULL},
#line 47 "src/keywords.gperf"
    {"__restrict__", ID___RESTRICT__, KW_QUAL, NULL},
#line 133 "src/keywords.gperf"
    {"ifdef", ID_IFDEF, KW_PREPROC, NULL},
#line 148 "src/keywords.gperf"
    {"llabs", ID_LLABS, KW_BUILTIN, NULL},
#line 29 "src/keywords.gperf"
    {"static", ID_STATIC, KW_STORAGE, NULL},
#line 158 "src/keywords.gperf"
    {"scanf", ID_SCANF, KW_BUILTIN, NULL},
#line 134 "src/keywords.gperf"
    {"ifndef", ID_IFNDEF, KW_PREPROC, NULL},
    {(char*)0},
#line 64 "src/keywords.gperf"
    {"_Decimal128", ID__DECIMAL128, KW_TYPE, NULL},
#line 125 "src/keywords.gperf"
    {"__asm__", ID___ASM__, KW_SPECIAL, NULL},
#line 21 "src/keywords.gperf"
    {"case", ID_CASE, KW_CONTROL, NULL},
    {(char*)0}, {(char*)0},
#line 217 "src/keywords.gperf"
    {"__builtin_signbitl", ID___BUILTIN_SIGNBITL, KW_BUILTIN, NULL},
#line 39 "src/keywords.gperf"
    {"const", ID_CONST, KW_QUAL, NULL},
#line 216 "src/keywords.gperf"
    {"__builtin_signbitf", ID___BUILTIN_SIGNBITF, KW_BUILTIN, NULL},
#line 86 "src/keywords.gperf"
    {"__alias__", ID___ALIAS__, KW_ATTR, NULL},
#line 176 "src/keywords.gperf"
    {"__builtin_clrsb", ID___BUILTIN_CLRSB, KW_BUILTIN, NULL},
#line 159 "src/keywords.gperf"
    {"fscanf", ID_FSCANF, KW_BUILTIN, NULL},
#line 19 "src/keywords.gperf"
    {"for", ID_FOR, KW_CONTROL, NULL},
#line 102 "src/keywords.gperf"
    {"__cdecl", ID___CDECL, KW_ASM, NULL},
    {(char*)0},
#line 160 "src/keywords.gperf"
    {"sscanf", ID_SSCANF, KW_BUILTIN, NULL},
    {(char*)0},
#line 135 "src/keywords.gperf"
    {"elif", ID_ELIF, KW_PREPROC, NULL},
#line 171 "src/keywords.gperf"
    {"__builtin_bswap16", ID___BUILTIN_BSWAP16, KW_BUILTIN, NULL},
#line 166 "src/keywords.gperf"
    {"strchr", ID_STRCHR, KW_BUILTIN, NULL},
    {(char*)0},
#line 215 "src/keywords.gperf"
    {"__builtin_signbit", ID___BUILTIN_SIGNBIT, KW_BUILTIN, NULL},
#line 136 "src/keywords.gperf"
    {"endif", ID_ENDIF, KW_PREPROC, NULL},
#line 175 "src/keywords.gperf"
    {"__builtin_classify_type", ID___BUILTIN_CLASSIFY_TYPE, KW_BUILTIN, NULL},
#line 57 "src/keywords.gperf"
    {"__int128", ID___INT128, KW_TYPE, NULL},
#line 41 "src/keywords.gperf"
    {"__const__", ID___CONST__, KW_QUAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 40 "src/keywords.gperf"
    {"__const", ID___CONST, KW_QUAL, NULL},
    {(char*)0},
#line 32 "src/keywords.gperf"
    {"__inline__", ID___INLINE__, KW_STORAGE, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 16 "src/keywords.gperf"
    {"else", ID_ELSE, KW_CONTROL, NULL},
#line 113 "src/keywords.gperf"
    {"__alignof", ID___ALIGNOF, KW_SPECIAL, NULL},
#line 78 "src/keywords.gperf"
    {"__signed__", ID___SIGNED__, KW_TYPE, NULL},
#line 18 "src/keywords.gperf"
    {"do", ID_DO, KW_CONTROL, NULL},
#line 53 "src/keywords.gperf"
    {"char", ID_CHAR, KW_TYPE, NULL},
#line 116 "src/keywords.gperf"
    {"_Pragma", ID__PRAGMA, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0},
#line 88 "src/keywords.gperf"
    {"__aligned__", ID___ALIGNED__, KW_ATTR, NULL},
#line 63 "src/keywords.gperf"
    {"_Decimal64", ID__DECIMAL64, KW_TYPE, NULL},
#line 132 "src/keywords.gperf"
    {"include", ID_INCLUDE, KW_PREPROC, NULL},
#line 114 "src/keywords.gperf"
    {"__alignof__", ID___ALIGNOF__, KW_SPECIAL, NULL},
#line 146 "src/keywords.gperf"
    {"abs", ID_ABS, KW_BUILTIN, NULL},
    {(char*)0},
#line 218 "src/keywords.gperf"
    {"__builtin_strchr", ID___BUILTIN_STRCHR, KW_BUILTIN, NULL},
#line 38 "src/keywords.gperf"
    {"_Thread_local", ID__THREAD_LOCAL, KW_STORAGE, NULL},
#line 62 "src/keywords.gperf"
    {"_Decimal32", ID__DECIMAL32, KW_TYPE, NULL},
#line 196 "src/keywords.gperf"
    {"__builtin_labs", ID___BUILTIN_LABS, KW_BUILTIN, NULL},
#line 204 "src/keywords.gperf"
    {"__builtin_offsetof", ID___BUILTIN_OFFSETOF, KW_SPECIAL, NULL},
#line 23 "src/keywords.gperf"
    {"break", ID_BREAK, KW_CONTROL, NULL},
    {(char*)0},
#line 31 "src/keywords.gperf"
    {"__inline", ID___INLINE, KW_STORAGE, NULL},
#line 45 "src/keywords.gperf"
    {"restrict", ID_RESTRICT, KW_QUAL, NULL},
#line 24 "src/keywords.gperf"
    {"continue", ID_CONTINUE, KW_CONTROL, NULL},
#line 61 "src/keywords.gperf"
    {"__complex__", ID___COMPLEX__, KW_TYPE, NULL},
    {(char*)0},
#line 85 "src/keywords.gperf"
    {"alias", ID_ALIAS, KW_ATTR, NULL},
    {(char*)0}, {(char*)0},
#line 89 "src/keywords.gperf"
    {"cleanup", ID_CLEANUP, KW_ATTR, NULL},
#line 59 "src/keywords.gperf"
    {"_Bool", ID__BOOL, KW_TYPE, NULL},
    {(char*)0},
#line 58 "src/keywords.gperf"
    {"__int64", ID___INT64, KW_TYPE, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 138 "src/keywords.gperf"
    {"once", ID_ONCE, KW_PREPROC, NULL},
#line 165 "src/keywords.gperf"
    {"strcmp", ID_STRCMP, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 55 "src/keywords.gperf"
    {"float", ID_FLOAT, KW_TYPE, NULL},
#line 83 "src/keywords.gperf"
    {"__attribute__", ID___ATTRIBUTE__, KW_ATTR, NULL},
#line 77 "src/keywords.gperf"
    {"__signed", ID___SIGNED, KW_TYPE, NULL},
#line 36 "src/keywords.gperf"
    {"__auto_type", ID___AUTO_TYPE, KW_STORAGE, NULL},
#line 80 "src/keywords.gperf"
    {"short", ID_SHORT, KW_TYPE, NULL},
    {(char*)0},
#line 51 "src/keywords.gperf"
    {"_Atomic", ID__ATOMIC, KW_QUAL, NULL},
#line 167 "src/keywords.gperf"
    {"__builtin_abs", ID___BUILTIN_ABS, KW_BUILTIN, NULL},
#line 231 "src/keywords.gperf"
    {"__atomic_clear", ID___ATOMIC_CLEAR, KW_ATOMIC, NULL},
#line 248 "src/keywords.gperf"
    {"__atomic_store", ID___ATOMIC_STORE, KW_ATOMIC, NULL},
    {(char*)0},
#line 71 "src/keywords.gperf"
    {"__typeof", ID___TYPEOF, KW_TYPE, NULL},
#line 20 "src/keywords.gperf"
    {"switch", ID_SWITCH, KW_CONTROL, NULL},
#line 150 "src/keywords.gperf"
    {"printf", ID_PRINTF, KW_BUILTIN, NULL},
#line 234 "src/keywords.gperf"
    {"__atomic_exchange", ID___ATOMIC_EXCHANGE, KW_ATOMIC, NULL},
#line 93 "src/keywords.gperf"
    {"destructor", ID_DESTRUCTOR, KW_ATTR, NULL},
#line 72 "src/keywords.gperf"
    {"__typeof__", ID___TYPEOF__, KW_TYPE, NULL},
    {(char*)0},
#line 52 "src/keywords.gperf"
    {"int", ID_INT, KW_TYPE, NULL},
#line 91 "src/keywords.gperf"
    {"constructor", ID_CONSTRUCTOR, KW_ATTR, NULL},
    {(char*)0},
#line 82 "src/keywords.gperf"
    {"__attribute", ID___ATTRIBUTE, KW_ATTR, NULL},
#line 164 "src/keywords.gperf"
    {"strlen", ID_STRLEN, KW_BUILTIN, NULL},
    {(char*)0},
#line 84 "src/keywords.gperf"
    {"__declspec", ID___DECLSPEC, KW_ATTR, NULL},
#line 219 "src/keywords.gperf"
    {"__builtin_strcmp", ID___BUILTIN_STRCMP, KW_BUILTIN, NULL},
#line 179 "src/keywords.gperf"
    {"__builtin_clz", ID___BUILTIN_CLZ, KW_BUILTIN, NULL},
#line 98 "src/keywords.gperf"
    {"__mode__", ID___MODE__, KW_ATTR, NULL},
    {(char*)0},
#line 87 "src/keywords.gperf"
    {"aligned", ID_ALIGNED, KW_ATTR, NULL},
    {(char*)0},
#line 242 "src/keywords.gperf"
    {"__atomic_is_lock_free", ID___ATOMIC_IS_LOCK_FREE, KW_ATOMIC, NULL},
#line 60 "src/keywords.gperf"
    {"_Complex", ID__COMPLEX, KW_TYPE, NULL},
#line 131 "src/keywords.gperf"
    {"undef", ID_UNDEF, KW_PREPROC, NULL},
#line 76 "src/keywords.gperf"
    {"signed", ID_SIGNED, KW_TYPE, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 142 "src/keywords.gperf"
    {"push_macro", ID_PUSH_MACRO, KW_PREPROC, NULL},
    {(char*)0},
#line 112 "src/keywords.gperf"
    {"_Alignof", ID__ALIGNOF, KW_SPECIAL, NULL},
#line 214 "src/keywords.gperf"
    {"__builtin_setjmp", ID___BUILTIN_SETJMP, KW_BUILTIN, NULL},
#line 186 "src/keywords.gperf"
    {"__builtin_ctz", ID___BUILTIN_CTZ, KW_BUILTIN, NULL},
#line 251 "src/keywords.gperf"
    {"__atomic_test_and_set", ID___ATOMIC_TEST_AND_SET, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 123 "src/keywords.gperf"
    {"__label__", ID___LABEL__, KW_SPECIAL, NULL},
#line 111 "src/keywords.gperf"
    {"_Alignas", ID__ALIGNAS, KW_ATTR, NULL},
#line 92 "src/keywords.gperf"
    {"__constructor__", ID___CONSTRUCTOR__, KW_ATTR, NULL},
#line 213 "src/keywords.gperf"
    {"__builtin_return_address", ID___BUILTIN_RETURN_ADDRESS, KW_BUILTIN, NULL},
#line 96 "src/keywords.gperf"
    {"ms_struct", ID_MS_STRUCT, KW_ATTR, NULL},
#line 220 "src/keywords.gperf"
    {"__builtin_strlen", ID___BUILTIN_STRLEN, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 155 "src/keywords.gperf"
    {"fputs", ID_FPUTS, KW_BUILTIN, NULL},
    {(char*)0},
#line 152 "src/keywords.gperf"
    {"vprintf", ID_VPRINTF, KW_BUILTIN, NULL},
#line 44 "src/keywords.gperf"
    {"__volatile__", ID___VOLATILE__, KW_QUAL, NULL},
    {(char*)0},
#line 144 "src/keywords.gperf"
    {"unicode", ID_UNICODE, KW_PREPROC, NULL},
    {(char*)0},
#line 241 "src/keywords.gperf"
    {"__atomic_fetch_xor", ID___ATOMIC_FETCH_XOR, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0},
#line 30 "src/keywords.gperf"
    {"inline", ID_INLINE, KW_STORAGE, NULL},
#line 124 "src/keywords.gperf"
    {"__asm", ID___ASM, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0},
#line 90 "src/keywords.gperf"
    {"__cleanup__", ID___CLEANUP__, KW_ATTR, NULL},
#line 35 "src/keywords.gperf"
    {"constexpr", ID_CONSTEXPR, KW_STORAGE, NULL},
#line 49 "src/keywords.gperf"
    {"_Static_assert", ID__STATIC_ASSERT, KW_SPECIAL, NULL},
    {(char*)0},
#line 43 "src/keywords.gperf"
    {"__volatile", ID___VOLATILE, KW_QUAL, NULL},
#line 206 "src/keywords.gperf"
    {"__builtin_parityl", ID___BUILTIN_PARITYL, KW_BUILTIN, NULL},
#line 207 "src/keywords.gperf"
    {"__builtin_parityll", ID___BUILTIN_PARITYLL, KW_BUILTIN, NULL},
#line 239 "src/keywords.gperf"
    {"__atomic_fetch_or", ID___ATOMIC_FETCH_OR, KW_ATOMIC, NULL},
#line 236 "src/keywords.gperf"
    {"__atomic_fetch_add", ID___ATOMIC_FETCH_ADD, KW_ATOMIC, NULL},
#line 174 "src/keywords.gperf"
    {"__builtin_choose_expr", ID___BUILTIN_CHOOSE_EXPR, KW_BUILTIN, NULL},
#line 105 "src/keywords.gperf"
    {"__thiscall", ID___THISCALL, KW_ASM, NULL},
#line 243 "src/keywords.gperf"
    {"__atomic_load", ID___ATOMIC_LOAD, KW_ATOMIC, NULL},
#line 120 "src/keywords.gperf"
    {"__func__", ID___FUNC__, KW_SPECIAL, NULL},
#line 99 "src/keywords.gperf"
    {"packed", ID_PACKED, KW_ATTR, NULL},
    {(char*)0},
#line 192 "src/keywords.gperf"
    {"__builtin_frame_address", ID___BUILTIN_FRAME_ADDRESS, KW_BUILTIN, NULL},
    {(char*)0},
#line 17 "src/keywords.gperf"
    {"while", ID_WHILE, KW_CONTROL, NULL},
#line 137 "src/keywords.gperf"
    {"pragma", ID_PRAGMA, KW_PREPROC, NULL},
#line 106 "src/keywords.gperf"
    {"__vectorcall", ID___VECTORCALL, KW_ASM, NULL},
    {(char*)0},
#line 97 "src/keywords.gperf"
    {"mode", ID_MODE, KW_ATTR, NULL},
#line 173 "src/keywords.gperf"
    {"__builtin_bswap64", ID___BUILTIN_BSWAP64, KW_BUILTIN, NULL},
#line 139 "src/keywords.gperf"
    {"pack", ID_PACK, KW_PREPROC, NULL},
#line 157 "src/keywords.gperf"
    {"snprintf", ID_SNPRINTF, KW_BUILTIN, NULL},
#line 48 "src/keywords.gperf"
    {"static_assert", ID_STATIC_ASSERT, KW_SPECIAL, NULL},
#line 140 "src/keywords.gperf"
    {"push", ID_PUSH, KW_PREPROC, NULL},
#line 201 "src/keywords.gperf"
    {"__builtin_memset", ID___BUILTIN_MEMSET, KW_BUILTIN, NULL},
#line 81 "src/keywords.gperf"
    {"long", ID_LONG, KW_TYPE, NULL},
    {(char*)0},
#line 70 "src/keywords.gperf"
    {"typeof", ID_TYPEOF, KW_TYPE, NULL},
#line 27 "src/keywords.gperf"
    {"typedef", ID_TYPEDEF, KW_STORAGE, NULL},
    {(char*)0}, {(char*)0},
#line 34 "src/keywords.gperf"
    {"auto", ID_AUTO, KW_STORAGE, NULL},
#line 260 "src/keywords.gperf"
    {"__sync_fetch_and_xor", ID___SYNC_FETCH_AND_XOR, KW_SYNC, NULL},
#line 172 "src/keywords.gperf"
    {"__builtin_bswap32", ID___BUILTIN_BSWAP32, KW_BUILTIN, NULL},
#line 37 "src/keywords.gperf"
    {"__thread", ID___THREAD, KW_STORAGE, NULL},
#line 154 "src/keywords.gperf"
    {"puts", ID_PUTS, KW_BUILTIN, NULL},
    {(char*)0},
#line 249 "src/keywords.gperf"
    {"__atomic_store_n", ID___ATOMIC_STORE_N, KW_ATOMIC, NULL},
    {(char*)0},
#line 212 "src/keywords.gperf"
    {"__builtin_return", ID___BUILTIN_RETURN, KW_BUILTIN, NULL},
    {(char*)0},
#line 253 "src/keywords.gperf"
    {"__atomic_xor_fetch", ID___ATOMIC_XOR_FETCH, KW_ATOMIC, NULL},
#line 261 "src/keywords.gperf"
    {"__sync_lock_release", ID___SYNC_LOCK_RELEASE, KW_SYNC, NULL},
#line 169 "src/keywords.gperf"
    {"__builtin_apply", ID___BUILTIN_APPLY, KW_BUILTIN, NULL},
#line 205 "src/keywords.gperf"
    {"__builtin_parity", ID___BUILTIN_PARITY, KW_BUILTIN, NULL},
#line 79 "src/keywords.gperf"
    {"unsigned", ID_UNSIGNED, KW_TYPE, NULL},
#line 258 "src/keywords.gperf"
    {"__sync_fetch_and_or", ID___SYNC_FETCH_AND_OR, KW_SYNC, NULL},
#line 255 "src/keywords.gperf"
    {"__sync_fetch_and_add", ID___SYNC_FETCH_AND_ADD, KW_SYNC, NULL},
    {(char*)0},
#line 101 "src/keywords.gperf"
    {"weak", ID_WEAK, KW_ATTR, NULL},
#line 185 "src/keywords.gperf"
    {"__builtin_copysignl", ID___BUILTIN_COPYSIGNL, KW_BUILTIN, NULL},
#line 200 "src/keywords.gperf"
    {"__builtin_memcpy", ID___BUILTIN_MEMCPY, KW_BUILTIN, NULL},
#line 54 "src/keywords.gperf"
    {"void", ID_VOID, KW_TYPE, NULL},
    {(char*)0},
#line 184 "src/keywords.gperf"
    {"__builtin_copysignf", ID___BUILTIN_COPYSIGNF, KW_BUILTIN, NULL},
    {(char*)0},
#line 229 "src/keywords.gperf"
    {"__atomic_add_fetch", ID___ATOMIC_ADD_FETCH, KW_ATOMIC, NULL},
#line 33 "src/keywords.gperf"
    {"register", ID_REGISTER, KW_STORAGE, NULL},
#line 56 "src/keywords.gperf"
    {"double", ID_DOUBLE, KW_TYPE, NULL},
#line 22 "src/keywords.gperf"
    {"default", ID_DEFAULT, KW_CONTROL, NULL},
#line 240 "src/keywords.gperf"
    {"__atomic_fetch_sub", ID___ATOMIC_FETCH_SUB, KW_ATOMIC, NULL},
#line 68 "src/keywords.gperf"
    {"_Float64x", ID__FLOAT64X, KW_TYPE, NULL},
#line 94 "src/keywords.gperf"
    {"__destructor__", ID___DESTRUCTOR__, KW_ATTR, NULL},
#line 69 "src/keywords.gperf"
    {"_Float128", ID__FLOAT128, KW_TYPE, NULL},
#line 262 "src/keywords.gperf"
    {"__sync_lock_test_and_set", ID___SYNC_LOCK_TEST_AND_SET, KW_SYNC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 66 "src/keywords.gperf"
    {"_Float32x", ID__FLOAT32X, KW_TYPE, NULL},
#line 257 "src/keywords.gperf"
    {"__sync_fetch_and_nand", ID___SYNC_FETCH_AND_NAND, KW_SYNC, NULL},
#line 198 "src/keywords.gperf"
    {"__builtin_longjmp", ID___BUILTIN_LONGJMP, KW_BUILTIN, NULL},
    {(char*)0},
#line 104 "src/keywords.gperf"
    {"__fastcall", ID___FASTCALL, KW_ASM, NULL},
#line 100 "src/keywords.gperf"
    {"__packed__", ID___PACKED__, KW_ATTR, NULL},
#line 130 "src/keywords.gperf"
    {"define", ID_DEFINE, KW_PREPROC, NULL},
    {(char*)0}, {(char*)0},
#line 238 "src/keywords.gperf"
    {"__atomic_fetch_nand", ID___ATOMIC_FETCH_NAND, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 211 "src/keywords.gperf"
    {"__builtin_prefetch", ID___BUILTIN_PREFETCH, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 74 "src/keywords.gperf"
    {"union", ID_UNION, KW_TYPE, NULL},
#line 237 "src/keywords.gperf"
    {"__atomic_fetch_and", ID___ATOMIC_FETCH_AND, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 28 "src/keywords.gperf"
    {"extern", ID_EXTERN, KW_STORAGE, NULL},
#line 244 "src/keywords.gperf"
    {"__atomic_load_n", ID___ATOMIC_LOAD_N, KW_ATOMIC, NULL},
    {(char*)0},
#line 145 "src/keywords.gperf"
    {"defined", ID_DEFINED, KW_PREPROC, NULL},
    {(char*)0},
#line 254 "src/keywords.gperf"
    {"__sync_bool_compare_and_swap", ID___SYNC_BOOL_COMPARE_AND_SWAP, KW_SYNC, NULL},
    {(char*)0}, {(char*)0},
#line 149 "src/keywords.gperf"
    {"alloca", ID_ALLOCA, KW_BUILTIN, NULL},
#line 209 "src/keywords.gperf"
    {"__builtin_popcountl", ID___BUILTIN_POPCOUNTL, KW_BUILTIN, NULL},
#line 210 "src/keywords.gperf"
    {"__builtin_popcountll", ID___BUILTIN_POPCOUNTLL, KW_BUILTIN, NULL},
    {(char*)0},
#line 259 "src/keywords.gperf"
    {"__sync_fetch_and_sub", ID___SYNC_FETCH_AND_SUB, KW_SYNC, NULL},
#line 161 "src/keywords.gperf"
    {"memset", ID_MEMSET, KW_BUILTIN, NULL},
    {(char*)0},
#line 182 "src/keywords.gperf"
    {"__builtin_constant_p", ID___BUILTIN_CONSTANT_P, KW_BUILTIN, NULL},
#line 263 "src/keywords.gperf"
    {"__sync_synchronize", ID___SYNC_SYNCHRONIZE, KW_SYNC, NULL},
#line 232 "src/keywords.gperf"
    {"__atomic_compare_exchange", ID___ATOMIC_COMPARE_EXCHANGE, KW_ATOMIC, NULL},
#line 25 "src/keywords.gperf"
    {"return", ID_RETURN, KW_CONTROL, NULL},
#line 246 "src/keywords.gperf"
    {"__atomic_or_fetch", ID___ATOMIC_OR_FETCH, KW_ATOMIC, NULL},
    {(char*)0},
#line 250 "src/keywords.gperf"
    {"__atomic_sub_fetch", ID___ATOMIC_SUB_FETCH, KW_ATOMIC, NULL},
#line 199 "src/keywords.gperf"
    {"__builtin_memcmp", ID___BUILTIN_MEMCMP, KW_BUILTIN, NULL},
    {(char*)0},
#line 208 "src/keywords.gperf"
    {"__builtin_popcount", ID___BUILTIN_POPCOUNT, KW_BUILTIN, NULL},
#line 67 "src/keywords.gperf"
    {"_Float64", ID__FLOAT64, KW_TYPE, NULL},
    {(char*)0},
#line 227 "src/keywords.gperf"
    {"__builtin_va_end", ID___BUILTIN_VA_END, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 256 "src/keywords.gperf"
    {"__sync_fetch_and_and", ID___SYNC_FETCH_AND_AND, KW_SYNC, NULL},
#line 65 "src/keywords.gperf"
    {"_Float32", ID__FLOAT32, KW_TYPE, NULL},
#line 162 "src/keywords.gperf"
    {"memcpy", ID_MEMCPY, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 228 "src/keywords.gperf"
    {"__builtin_va_start", ID___BUILTIN_VA_START, KW_SPECIAL, NULL},
#line 168 "src/keywords.gperf"
    {"__builtin_add_overflow", ID___BUILTIN_ADD_OVERFLOW, KW_BUILTIN, NULL},
#line 230 "src/keywords.gperf"
    {"__atomic_and_fetch", ID___ATOMIC_AND_FETCH, KW_ATOMIC, NULL},
#line 50 "src/keywords.gperf"
    {"nullptr", ID_NULLPTR, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 221 "src/keywords.gperf"
    {"__builtin_sub_overflow", ID___BUILTIN_SUB_OVERFLOW, KW_BUILTIN, NULL},
    {(char*)0},
#line 266 "src/keywords.gperf"
    {"__vprintf_chk", ID___VPRINTF_CHK, KW_BUILTIN, NULL},
    {(char*)0},
#line 26 "src/keywords.gperf"
    {"goto", ID_GOTO, KW_CONTROL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 222 "src/keywords.gperf"
    {"__builtin_types_compatible_p", ID___BUILTIN_TYPES_COMPATIBLE_P, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0},
#line 226 "src/keywords.gperf"
    {"__builtin_va_copy", ID___BUILTIN_VA_COPY, KW_SPECIAL, NULL},
#line 126 "src/keywords.gperf"
    {"asm", ID_ASM, KW_SPECIAL, NULL},
#line 75 "src/keywords.gperf"
    {"enum", ID_ENUM, KW_TYPE, NULL},
#line 121 "src/keywords.gperf"
    {"__has_include", ID___HAS_INCLUDE, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 268 "src/keywords.gperf"
    {"__vfprintf_chk", ID___VFPRINTF_CHK, KW_BUILTIN, NULL},
#line 122 "src/keywords.gperf"
    {"__has_include_next", ID___HAS_INCLUDE_NEXT, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 235 "src/keywords.gperf"
    {"__atomic_exchange_n", ID___ATOMIC_EXCHANGE_N, KW_ATOMIC, NULL},
#line 252 "src/keywords.gperf"
    {"__atomic_thread_fence", ID___ATOMIC_THREAD_FENCE, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 183 "src/keywords.gperf"
    {"__builtin_copysign", ID___BUILTIN_COPYSIGN, KW_BUILTIN, NULL},
#line 267 "src/keywords.gperf"
    {"__fprintf_chk", ID___FPRINTF_CHK, KW_BUILTIN, NULL},
#line 264 "src/keywords.gperf"
    {"__sync_val_compare_and_swap", ID___SYNC_VAL_COMPARE_AND_SWAP, KW_SYNC, NULL},
#line 95 "src/keywords.gperf"
    {"gcc_struct", ID_GCC_STRUCT, KW_ATTR, NULL},
    {(char*)0},
#line 153 "src/keywords.gperf"
    {"vfprintf", ID_VFPRINTF, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 163 "src/keywords.gperf"
    {"memcmp", ID_MEMCMP, KW_BUILTIN, NULL},
#line 42 "src/keywords.gperf"
    {"volatile", ID_VOLATILE, KW_QUAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 143 "src/keywords.gperf"
    {"pop_macro", ID_POP_MACRO, KW_PREPROC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 233 "src/keywords.gperf"
    {"__atomic_compare_exchange_n", ID___ATOMIC_COMPARE_EXCHANGE_N, KW_ATOMIC, NULL},
    {(char*)0},
#line 224 "src/keywords.gperf"
    {"__builtin_va_arg_pack", ID___BUILTIN_VA_ARG_PACK, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 141 "src/keywords.gperf"
    {"pop", ID_POP, KW_PREPROC, NULL},
    {(char*)0},
#line 247 "src/keywords.gperf"
    {"__atomic_signal_fence", ID___ATOMIC_SIGNAL_FENCE, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 203 "src/keywords.gperf"
    {"__builtin_mul_overflow_p", ID___BUILTIN_MUL_OVERFLOW_P, KW_BUILTIN, NULL},
    {(char*)0},
#line 265 "src/keywords.gperf"
    {"__printf_chk", ID___PRINTF_CHK, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 223 "src/keywords.gperf"
    {"__builtin_va_arg", ID___BUILTIN_VA_ARG, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 170 "src/keywords.gperf"
    {"__builtin_apply_args", ID___BUILTIN_APPLY_ARGS, KW_BUILTIN, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 245 "src/keywords.gperf"
    {"__atomic_nand_fetch", ID___ATOMIC_NAND_FETCH, KW_ATOMIC, NULL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 225 "src/keywords.gperf"
    {"__builtin_va_arg_pack_len", ID___BUILTIN_VA_ARG_PACK_LEN, KW_SPECIAL, NULL},
    {(char*)0}, {(char*)0},
#line 202 "src/keywords.gperf"
    {"__builtin_mul_overflow", ID___BUILTIN_MUL_OVERFLOW, KW_BUILTIN, NULL}
  };
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic pop
#endif

const struct keyword_entry *
keyword_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = keyword_table[key].name;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &keyword_table[key];
          }
    }
  return (struct keyword_entry *) 0;
}
