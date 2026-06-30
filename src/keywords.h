/* C code produced by gperf version 3.0.3 */
/* Command-line: /Library/Developer/CommandLineTools/usr/bin/gperf -m 10 --output-file=src/keywords.h.tmp src/keywords.gperf  */
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
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "src/keywords.gperf"

#include "keyword_ids.h"
#include <limits.h>
#line 6 "src/keywords.gperf"
struct keyword_entry { const char *name; int id; unsigned flags; char *interned; };

#define TOTAL_KEYWORDS 249
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 28
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 496
/* maximum key range = 489, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     const char *str;
     size_t len;
{
  static const unsigned short asso_values[] =
    {
      497, 497, 497, 497, 497, 497, 497, 497, 497, 497,
      497, 497, 497, 497, 497, 497, 497, 497, 497, 497,
      497, 497, 497, 497, 497, 497, 497, 497, 497, 497,
      497, 497, 497, 497, 497, 497, 497, 497, 497, 497,
      497, 497, 497, 497, 497, 497, 497, 497, 497, 497,
      113, 497,  67, 497,  11, 497,   3, 497, 497, 497,
      497, 497, 497, 497, 497, 497, 497, 497,   6, 497,
      497,   9,   5,   3, 497, 497,   3,   3,   4, 497,
      497,   4, 497,   3, 497,   4, 497, 497, 497,   4,
      497, 497, 497, 497, 497,   8,   3,  74,   5,   3,
       39,  39,   5, 217, 111,   5, 234,  77,   3, 161,
      104,  44,  86, 148,  50,   9,  29,  69,  70, 111,
       84,  32,  42,   5, 497, 497, 497, 497, 497
    };
  unsigned int hval = len & UINT_MAX;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[18]];
      /*FALLTHROUGH*/
      case 18:
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      /*FALLTHROUGH*/
      case 16:
      case 15:
      case 14:
      case 13:
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
      case 9:
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
        hval += asso_values[(unsigned char)str[2]+1];
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
     0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2,  0,
     4,  0,  0,  1,  0,  4,  0,  0,  5,  6,  5,  6,  7,  8,
     7,  0,  7,  0,  0, 11,  0,  8,  0,  0,  9, 14, 15, 16,
    15, 14, 15, 17, 15,  0, 13, 16, 15, 16,  9,  6,  5,  6,
    12,  7, 12,  6,  6, 10,  4, 19,  0, 14, 15,  3,  0, 17,
     0,  9,  9,  0,  6,  4,  0, 13,  0, 11, 18,  5, 18,  7,
    11,  2, 13,  7,  5,  5,  7,  7,  5, 10, 10,  8,  0, 23,
     7,  0, 10,  7,  5, 13,  0, 17,  3, 11,  0,  0,  6,  4,
     0, 11, 13,  3, 14, 13,  5,  0, 16,  0, 14,  8,  0,  5,
     4,  0,  6,  5,  0,  0, 18,  8,  0,  0,  8,  0,  0, 10,
     0,  0, 14, 11,  0,  8, 10,  0,  0,  0,  0, 21,  0,  0,
     6,  0, 16, 10,  0,  0, 12, 11, 21,  8, 10,  6, 16,  0,
     4,  0,  6,  6, 10, 10, 16,  8, 18,  0,  0,  5, 18, 17,
     0,  0,  8, 24, 13,  8,  6,  7, 12,  4,  0, 15,  9,  8,
    18,  4, 17, 17, 18, 17,  0,  0,  0,  9,  4,  0,  0, 16,
     0,  6,  0, 10,  0, 16, 23,  0,  6,  0,  0,  0, 18, 15,
    16, 16, 18,  0,  0,  6,  4,  0, 14,  0,  0,  0,  8,  0,
     0,  0,  0, 17, 18,  4,  0, 18,  0,  0,  0,  8,  0,  0,
    17, 15,  0, 19,  8, 18, 20, 18,  0, 16, 20,  0, 16,  0,
     0,  0,  4,  8,  8, 22,  0, 17,  4, 19, 20, 13, 20,  0,
    28, 20,  4, 19, 18,  0,  0,  0,  0, 27, 24, 18,  7, 21,
     9,  0, 19, 18, 21, 10,  6,  6,  7,  6,  0, 19, 22,  0,
     0, 19,  0,  8, 10, 17,  0,  6,  7,  8, 16,  0,  0,  0,
     5,  3,  0, 14,  0, 20,  8,  9,  0,  6,  0,  0,  4,  0,
     0,  9,  0,  0, 21,  0,  3,  0,  0,  0, 21,  4,  0,  7,
     0,  0, 13,  0,  7, 28, 18,  6,  0,  0,  6,  0,  0,  0,
    10,  0,  0,  0,  0, 19,  0, 25,  0,  0,  0,  9,  0, 25,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,
     0, 16,  0,  0,  0,  0,  0,  0, 24,  0, 18,  8,  0,  0,
     0, 21,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0, 22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0, 12,  0,  0,  0,  0,  0, 27,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13,  0,  0,  0,
     0,  0, 19,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0, 20
  };

static const struct keyword_entry keyword_table[] =
  {
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 104 "src/keywords.gperf"
    {"SI", ID_SI, KW_MODE},
#line 102 "src/keywords.gperf"
    {"QI", ID_QI, KW_MODE},
#line 103 "src/keywords.gperf"
    {"HI", ID_HI, KW_MODE},
#line 105 "src/keywords.gperf"
    {"DI", ID_DI, KW_MODE},
#line 15 "src/keywords.gperf"
    {"if", ID_IF, KW_CONTROL},
    {(char*)0},
#line 123 "src/keywords.gperf"
    {"NULL", ID_NULL, KW_SPECIAL},
    {(char*)0}, {(char*)0},
#line 122 "src/keywords.gperf"
    {"_", ID__, KW_SPECIAL},
    {(char*)0},
#line 142 "src/keywords.gperf"
    {"labs", ID_LABS, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 143 "src/keywords.gperf"
    {"llabs", ID_LLABS, KW_BUILTIN},
#line 29 "src/keywords.gperf"
    {"static", ID_STATIC, KW_STORAGE},
#line 153 "src/keywords.gperf"
    {"scanf", ID_SCANF, KW_BUILTIN},
#line 112 "src/keywords.gperf"
    {"sizeof", ID_SIZEOF, KW_SPECIAL},
#line 146 "src/keywords.gperf"
    {"fprintf", ID_FPRINTF, KW_BUILTIN},
#line 110 "src/keywords.gperf"
    {"_Generic", ID__GENERIC, KW_SPECIAL},
#line 120 "src/keywords.gperf"
    {"__asm__", ID___ASM__, KW_SPECIAL},
    {(char*)0},
#line 151 "src/keywords.gperf"
    {"sprintf", ID_SPRINTF, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 59 "src/keywords.gperf"
    {"_Decimal128", ID__DECIMAL128, KW_TYPE},
    {(char*)0},
#line 124 "src/keywords.gperf"
    {"__retbuf", ID___RETBUF, KW_SPECIAL},
    {(char*)0}, {(char*)0},
#line 81 "src/keywords.gperf"
    {"__alias__", ID___ALIAS__, KW_ATTR},
#line 175 "src/keywords.gperf"
    {"__builtin_clzl", ID___BUILTIN_CLZL, KW_BUILTIN},
#line 176 "src/keywords.gperf"
    {"__builtin_clzll", ID___BUILTIN_CLZLL, KW_BUILTIN},
#line 172 "src/keywords.gperf"
    {"__builtin_clrsbl", ID___BUILTIN_CLRSBL, KW_BUILTIN},
#line 171 "src/keywords.gperf"
    {"__builtin_clrsb", ID___BUILTIN_CLRSB, KW_BUILTIN},
#line 185 "src/keywords.gperf"
    {"__builtin_ffsl", ID___BUILTIN_FFSL, KW_BUILTIN},
#line 186 "src/keywords.gperf"
    {"__builtin_ffsll", ID___BUILTIN_FFSLL, KW_BUILTIN},
#line 173 "src/keywords.gperf"
    {"__builtin_clrsbll", ID___BUILTIN_CLRSBLL, KW_BUILTIN},
#line 192 "src/keywords.gperf"
    {"__builtin_llabs", ID___BUILTIN_LLABS, KW_BUILTIN},
    {(char*)0},
#line 184 "src/keywords.gperf"
    {"__builtin_ffs", ID___BUILTIN_FFS, KW_BUILTIN},
#line 190 "src/keywords.gperf"
    {"__builtin_isinfl", ID___BUILTIN_ISINFL, KW_BUILTIN},
#line 188 "src/keywords.gperf"
    {"__builtin_isinf", ID___BUILTIN_ISINF, KW_BUILTIN},
#line 189 "src/keywords.gperf"
    {"__builtin_isinff", ID___BUILTIN_ISINFF, KW_BUILTIN},
#line 98 "src/keywords.gperf"
    {"__stdcall", ID___STDCALL, KW_ASM},
#line 68 "src/keywords.gperf"
    {"struct", ID_STRUCT, KW_TYPE},
#line 128 "src/keywords.gperf"
    {"ifdef", ID_IFDEF, KW_PREPROC},
#line 154 "src/keywords.gperf"
    {"fscanf", ID_FSCANF, KW_BUILTIN},
#line 113 "src/keywords.gperf"
    {"__FUNCTION__", ID___FUNCTION__, KW_SPECIAL},
#line 97 "src/keywords.gperf"
    {"__cdecl", ID___CDECL, KW_ASM},
#line 45 "src/keywords.gperf"
    {"__restrict__", ID___RESTRICT__, KW_QUAL},
#line 155 "src/keywords.gperf"
    {"sscanf", ID_SSCANF, KW_BUILTIN},
#line 129 "src/keywords.gperf"
    {"ifndef", ID_IFNDEF, KW_PREPROC},
#line 44 "src/keywords.gperf"
    {"__restrict", ID___RESTRICT, KW_QUAL},
#line 48 "src/keywords.gperf"
    {"char", ID_CHAR, KW_TYPE},
#line 114 "src/keywords.gperf"
    {"__PRETTY_FUNCTION__", ID___PRETTY_FUNCTION__, KW_SPECIAL},
    {(char*)0},
#line 182 "src/keywords.gperf"
    {"__builtin_ctzl", ID___BUILTIN_CTZL, KW_BUILTIN},
#line 183 "src/keywords.gperf"
    {"__builtin_ctzll", ID___BUILTIN_CTZLL, KW_BUILTIN},
#line 19 "src/keywords.gperf"
    {"for", ID_FOR, KW_CONTROL},
    {(char*)0},
#line 166 "src/keywords.gperf"
    {"__builtin_bswap16", ID___BUILTIN_BSWAP16, KW_BUILTIN},
    {(char*)0},
#line 108 "src/keywords.gperf"
    {"__alignof", ID___ALIGNOF, KW_SPECIAL},
#line 39 "src/keywords.gperf"
    {"__const__", ID___CONST__, KW_QUAL},
    {(char*)0},
#line 161 "src/keywords.gperf"
    {"strchr", ID_STRCHR, KW_BUILTIN},
#line 21 "src/keywords.gperf"
    {"case", ID_CASE, KW_CONTROL},
    {(char*)0},
#line 174 "src/keywords.gperf"
    {"__builtin_clz", ID___BUILTIN_CLZ, KW_BUILTIN},
    {(char*)0},
#line 83 "src/keywords.gperf"
    {"__aligned__", ID___ALIGNED__, KW_ATTR},
#line 212 "src/keywords.gperf"
    {"__builtin_signbitl", ID___BUILTIN_SIGNBITL, KW_BUILTIN},
#line 37 "src/keywords.gperf"
    {"const", ID_CONST, KW_QUAL},
#line 211 "src/keywords.gperf"
    {"__builtin_signbitf", ID___BUILTIN_SIGNBITF, KW_BUILTIN},
#line 38 "src/keywords.gperf"
    {"__const", ID___CONST, KW_QUAL},
#line 109 "src/keywords.gperf"
    {"__alignof__", ID___ALIGNOF__, KW_SPECIAL},
#line 18 "src/keywords.gperf"
    {"do", ID_DO, KW_CONTROL},
#line 78 "src/keywords.gperf"
    {"__attribute__", ID___ATTRIBUTE__, KW_ATTR},
#line 46 "src/keywords.gperf"
    {"_Atomic", ID__ATOMIC, KW_QUAL},
#line 131 "src/keywords.gperf"
    {"endif", ID_ENDIF, KW_PREPROC},
#line 150 "src/keywords.gperf"
    {"fputs", ID_FPUTS, KW_BUILTIN},
#line 127 "src/keywords.gperf"
    {"include", ID_INCLUDE, KW_PREPROC},
#line 147 "src/keywords.gperf"
    {"vprintf", ID_VPRINTF, KW_BUILTIN},
#line 23 "src/keywords.gperf"
    {"break", ID_BREAK, KW_CONTROL},
#line 58 "src/keywords.gperf"
    {"_Decimal64", ID__DECIMAL64, KW_TYPE},
#line 73 "src/keywords.gperf"
    {"__signed__", ID___SIGNED__, KW_TYPE},
#line 66 "src/keywords.gperf"
    {"__typeof", ID___TYPEOF, KW_TYPE},
    {(char*)0},
#line 170 "src/keywords.gperf"
    {"__builtin_classify_type", ID___BUILTIN_CLASSIFY_TYPE, KW_BUILTIN},
#line 111 "src/keywords.gperf"
    {"_Pragma", ID__PRAGMA, KW_SPECIAL},
    {(char*)0},
#line 67 "src/keywords.gperf"
    {"__typeof__", ID___TYPEOF__, KW_TYPE},
#line 84 "src/keywords.gperf"
    {"cleanup", ID_CLEANUP, KW_ATTR},
#line 54 "src/keywords.gperf"
    {"_Bool", ID__BOOL, KW_TYPE},
#line 181 "src/keywords.gperf"
    {"__builtin_ctz", ID___BUILTIN_CTZ, KW_BUILTIN},
    {(char*)0},
#line 210 "src/keywords.gperf"
    {"__builtin_signbit", ID___BUILTIN_SIGNBIT, KW_BUILTIN},
#line 47 "src/keywords.gperf"
    {"int", ID_INT, KW_TYPE},
#line 77 "src/keywords.gperf"
    {"__attribute", ID___ATTRIBUTE, KW_ATTR},
    {(char*)0}, {(char*)0},
#line 160 "src/keywords.gperf"
    {"strcmp", ID_STRCMP, KW_BUILTIN},
#line 16 "src/keywords.gperf"
    {"else", ID_ELSE, KW_CONTROL},
    {(char*)0},
#line 56 "src/keywords.gperf"
    {"__complex__", ID___COMPLEX__, KW_TYPE},
#line 36 "src/keywords.gperf"
    {"_Thread_local", ID__THREAD_LOCAL, KW_STORAGE},
#line 141 "src/keywords.gperf"
    {"abs", ID_ABS, KW_BUILTIN},
#line 191 "src/keywords.gperf"
    {"__builtin_labs", ID___BUILTIN_LABS, KW_BUILTIN},
#line 162 "src/keywords.gperf"
    {"__builtin_abs", ID___BUILTIN_ABS, KW_BUILTIN},
#line 126 "src/keywords.gperf"
    {"undef", ID_UNDEF, KW_PREPROC},
    {(char*)0},
#line 213 "src/keywords.gperf"
    {"__builtin_strchr", ID___BUILTIN_STRCHR, KW_BUILTIN},
    {(char*)0},
#line 226 "src/keywords.gperf"
    {"__atomic_clear", ID___ATOMIC_CLEAR, KW_ATOMIC},
#line 72 "src/keywords.gperf"
    {"__signed", ID___SIGNED, KW_TYPE},
    {(char*)0},
#line 50 "src/keywords.gperf"
    {"float", ID_FLOAT, KW_TYPE},
#line 133 "src/keywords.gperf"
    {"once", ID_ONCE, KW_PREPROC},
    {(char*)0},
#line 159 "src/keywords.gperf"
    {"strlen", ID_STRLEN, KW_BUILTIN},
#line 75 "src/keywords.gperf"
    {"short", ID_SHORT, KW_TYPE},
    {(char*)0}, {(char*)0},
#line 199 "src/keywords.gperf"
    {"__builtin_offsetof", ID___BUILTIN_OFFSETOF, KW_SPECIAL},
#line 24 "src/keywords.gperf"
    {"continue", ID_CONTINUE, KW_CONTROL},
    {(char*)0}, {(char*)0},
#line 93 "src/keywords.gperf"
    {"__mode__", ID___MODE__, KW_ATTR},
    {(char*)0}, {(char*)0},
#line 57 "src/keywords.gperf"
    {"_Decimal32", ID__DECIMAL32, KW_TYPE},
    {(char*)0}, {(char*)0},
#line 243 "src/keywords.gperf"
    {"__atomic_store", ID___ATOMIC_STORE, KW_ATOMIC},
#line 85 "src/keywords.gperf"
    {"__cleanup__", ID___CLEANUP__, KW_ATTR},
    {(char*)0},
#line 43 "src/keywords.gperf"
    {"restrict", ID_RESTRICT, KW_QUAL},
#line 79 "src/keywords.gperf"
    {"__declspec", ID___DECLSPEC, KW_ATTR},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 237 "src/keywords.gperf"
    {"__atomic_is_lock_free", ID___ATOMIC_IS_LOCK_FREE, KW_ATOMIC},
    {(char*)0}, {(char*)0},
#line 51 "src/keywords.gperf"
    {"double", ID_DOUBLE, KW_TYPE},
    {(char*)0},
#line 214 "src/keywords.gperf"
    {"__builtin_strcmp", ID___BUILTIN_STRCMP, KW_BUILTIN},
#line 88 "src/keywords.gperf"
    {"destructor", ID_DESTRUCTOR, KW_ATTR},
    {(char*)0}, {(char*)0},
#line 42 "src/keywords.gperf"
    {"__volatile__", ID___VOLATILE__, KW_QUAL},
#line 86 "src/keywords.gperf"
    {"constructor", ID_CONSTRUCTOR, KW_ATTR},
#line 246 "src/keywords.gperf"
    {"__atomic_test_and_set", ID___ATOMIC_TEST_AND_SET, KW_ATOMIC},
#line 35 "src/keywords.gperf"
    {"__thread", ID___THREAD, KW_STORAGE},
#line 100 "src/keywords.gperf"
    {"__thiscall", ID___THISCALL, KW_ASM},
#line 71 "src/keywords.gperf"
    {"signed", ID_SIGNED, KW_TYPE},
#line 209 "src/keywords.gperf"
    {"__builtin_setjmp", ID___BUILTIN_SETJMP, KW_BUILTIN},
    {(char*)0},
#line 149 "src/keywords.gperf"
    {"puts", ID_PUTS, KW_BUILTIN},
    {(char*)0},
#line 94 "src/keywords.gperf"
    {"packed", ID_PACKED, KW_ATTR},
#line 132 "src/keywords.gperf"
    {"pragma", ID_PRAGMA, KW_PREPROC},
#line 137 "src/keywords.gperf"
    {"push_macro", ID_PUSH_MACRO, KW_PREPROC},
#line 41 "src/keywords.gperf"
    {"__volatile", ID___VOLATILE, KW_QUAL},
#line 215 "src/keywords.gperf"
    {"__builtin_strlen", ID___BUILTIN_STRLEN, KW_BUILTIN},
#line 152 "src/keywords.gperf"
    {"snprintf", ID_SNPRINTF, KW_BUILTIN},
#line 235 "src/keywords.gperf"
    {"__atomic_fetch_sub", ID___ATOMIC_FETCH_SUB, KW_ATOMIC},
    {(char*)0}, {(char*)0},
#line 119 "src/keywords.gperf"
    {"__asm", ID___ASM, KW_SPECIAL},
#line 231 "src/keywords.gperf"
    {"__atomic_fetch_add", ID___ATOMIC_FETCH_ADD, KW_ATOMIC},
#line 168 "src/keywords.gperf"
    {"__builtin_bswap64", ID___BUILTIN_BSWAP64, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 74 "src/keywords.gperf"
    {"unsigned", ID_UNSIGNED, KW_TYPE},
#line 208 "src/keywords.gperf"
    {"__builtin_return_address", ID___BUILTIN_RETURN_ADDRESS, KW_BUILTIN},
#line 238 "src/keywords.gperf"
    {"__atomic_load", ID___ATOMIC_LOAD, KW_ATOMIC},
#line 107 "src/keywords.gperf"
    {"_Alignof", ID__ALIGNOF, KW_SPECIAL},
#line 65 "src/keywords.gperf"
    {"typeof", ID_TYPEOF, KW_TYPE},
#line 27 "src/keywords.gperf"
    {"typedef", ID_TYPEDEF, KW_STORAGE},
#line 101 "src/keywords.gperf"
    {"__vectorcall", ID___VECTORCALL, KW_ASM},
#line 34 "src/keywords.gperf"
    {"auto", ID_AUTO, KW_STORAGE},
    {(char*)0},
#line 87 "src/keywords.gperf"
    {"__constructor__", ID___CONSTRUCTOR__, KW_ATTR},
#line 118 "src/keywords.gperf"
    {"__label__", ID___LABEL__, KW_SPECIAL},
#line 106 "src/keywords.gperf"
    {"_Alignas", ID__ALIGNAS, KW_ATTR},
#line 236 "src/keywords.gperf"
    {"__atomic_fetch_xor", ID___ATOMIC_FETCH_XOR, KW_ATOMIC},
#line 96 "src/keywords.gperf"
    {"weak", ID_WEAK, KW_ATTR},
#line 229 "src/keywords.gperf"
    {"__atomic_exchange", ID___ATOMIC_EXCHANGE, KW_ATOMIC},
#line 201 "src/keywords.gperf"
    {"__builtin_parityl", ID___BUILTIN_PARITYL, KW_BUILTIN},
#line 202 "src/keywords.gperf"
    {"__builtin_parityll", ID___BUILTIN_PARITYLL, KW_BUILTIN},
#line 234 "src/keywords.gperf"
    {"__atomic_fetch_or", ID___ATOMIC_FETCH_OR, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 91 "src/keywords.gperf"
    {"ms_struct", ID_MS_STRUCT, KW_ATTR},
#line 134 "src/keywords.gperf"
    {"pack", ID_PACK, KW_PREPROC},
    {(char*)0}, {(char*)0},
#line 244 "src/keywords.gperf"
    {"__atomic_store_n", ID___ATOMIC_STORE_N, KW_ATOMIC},
    {(char*)0},
#line 30 "src/keywords.gperf"
    {"inline", ID_INLINE, KW_STORAGE},
    {(char*)0},
#line 95 "src/keywords.gperf"
    {"__packed__", ID___PACKED__, KW_ATTR},
    {(char*)0},
#line 222 "src/keywords.gperf"
    {"__builtin_va_end", ID___BUILTIN_VA_END, KW_SPECIAL},
#line 187 "src/keywords.gperf"
    {"__builtin_frame_address", ID___BUILTIN_FRAME_ADDRESS, KW_BUILTIN},
    {(char*)0},
#line 28 "src/keywords.gperf"
    {"extern", ID_EXTERN, KW_STORAGE},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 245 "src/keywords.gperf"
    {"__atomic_sub_fetch", ID___ATOMIC_SUB_FETCH, KW_ATOMIC},
#line 164 "src/keywords.gperf"
    {"__builtin_apply", ID___BUILTIN_APPLY, KW_BUILTIN},
#line 200 "src/keywords.gperf"
    {"__builtin_parity", ID___BUILTIN_PARITY, KW_BUILTIN},
#line 207 "src/keywords.gperf"
    {"__builtin_return", ID___BUILTIN_RETURN, KW_BUILTIN},
#line 224 "src/keywords.gperf"
    {"__atomic_add_fetch", ID___ATOMIC_ADD_FETCH, KW_ATOMIC},
    {(char*)0}, {(char*)0},
#line 25 "src/keywords.gperf"
    {"return", ID_RETURN, KW_CONTROL},
#line 135 "src/keywords.gperf"
    {"push", ID_PUSH, KW_PREPROC},
    {(char*)0},
#line 89 "src/keywords.gperf"
    {"__destructor__", ID___DESTRUCTOR__, KW_ATTR},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 148 "src/keywords.gperf"
    {"vfprintf", ID_VFPRINTF, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 221 "src/keywords.gperf"
    {"__builtin_va_copy", ID___BUILTIN_VA_COPY, KW_SPECIAL},
#line 248 "src/keywords.gperf"
    {"__atomic_xor_fetch", ID___ATOMIC_XOR_FETCH, KW_ATOMIC},
#line 92 "src/keywords.gperf"
    {"mode", ID_MODE, KW_ATTR},
    {(char*)0},
#line 232 "src/keywords.gperf"
    {"__atomic_fetch_and", ID___ATOMIC_FETCH_AND, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 115 "src/keywords.gperf"
    {"__func__", ID___FUNC__, KW_SPECIAL},
    {(char*)0}, {(char*)0},
#line 193 "src/keywords.gperf"
    {"__builtin_longjmp", ID___BUILTIN_LONGJMP, KW_BUILTIN},
#line 239 "src/keywords.gperf"
    {"__atomic_load_n", ID___ATOMIC_LOAD_N, KW_ATOMIC},
    {(char*)0},
#line 233 "src/keywords.gperf"
    {"__atomic_fetch_nand", ID___ATOMIC_FETCH_NAND, KW_ATOMIC},
#line 52 "src/keywords.gperf"
    {"__int128", ID___INT128, KW_TYPE},
#line 223 "src/keywords.gperf"
    {"__builtin_va_start", ID___BUILTIN_VA_START, KW_SPECIAL},
#line 254 "src/keywords.gperf"
    {"__sync_fetch_and_sub", ID___SYNC_FETCH_AND_SUB, KW_SYNC},
#line 258 "src/keywords.gperf"
    {"__sync_synchronize", ID___SYNC_SYNCHRONIZE, KW_SYNC},
    {(char*)0},
#line 196 "src/keywords.gperf"
    {"__builtin_memset", ID___BUILTIN_MEMSET, KW_BUILTIN},
#line 250 "src/keywords.gperf"
    {"__sync_fetch_and_add", ID___SYNC_FETCH_AND_ADD, KW_SYNC},
    {(char*)0},
#line 195 "src/keywords.gperf"
    {"__builtin_memcpy", ID___BUILTIN_MEMCPY, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 76 "src/keywords.gperf"
    {"long", ID_LONG, KW_TYPE},
#line 33 "src/keywords.gperf"
    {"register", ID_REGISTER, KW_STORAGE},
#line 55 "src/keywords.gperf"
    {"_Complex", ID__COMPLEX, KW_TYPE},
#line 216 "src/keywords.gperf"
    {"__builtin_sub_overflow", ID___BUILTIN_SUB_OVERFLOW, KW_BUILTIN},
    {(char*)0},
#line 167 "src/keywords.gperf"
    {"__builtin_bswap32", ID___BUILTIN_BSWAP32, KW_BUILTIN},
#line 70 "src/keywords.gperf"
    {"enum", ID_ENUM, KW_TYPE},
#line 204 "src/keywords.gperf"
    {"__builtin_popcountl", ID___BUILTIN_POPCOUNTL, KW_BUILTIN},
#line 205 "src/keywords.gperf"
    {"__builtin_popcountll", ID___BUILTIN_POPCOUNTLL, KW_BUILTIN},
#line 116 "src/keywords.gperf"
    {"__has_include", ID___HAS_INCLUDE, KW_SPECIAL},
#line 255 "src/keywords.gperf"
    {"__sync_fetch_and_xor", ID___SYNC_FETCH_AND_XOR, KW_SYNC},
    {(char*)0},
#line 249 "src/keywords.gperf"
    {"__sync_bool_compare_and_swap", ID___SYNC_BOOL_COMPARE_AND_SWAP, KW_SYNC},
#line 177 "src/keywords.gperf"
    {"__builtin_constant_p", ID___BUILTIN_CONSTANT_P, KW_BUILTIN},
#line 130 "src/keywords.gperf"
    {"elif", ID_ELIF, KW_PREPROC},
#line 253 "src/keywords.gperf"
    {"__sync_fetch_and_or", ID___SYNC_FETCH_AND_OR, KW_SYNC},
#line 206 "src/keywords.gperf"
    {"__builtin_prefetch", ID___BUILTIN_PREFETCH, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 259 "src/keywords.gperf"
    {"__sync_val_compare_and_swap", ID___SYNC_VAL_COMPARE_AND_SWAP, KW_SYNC},
#line 257 "src/keywords.gperf"
    {"__sync_lock_test_and_set", ID___SYNC_LOCK_TEST_AND_SET, KW_SYNC},
#line 225 "src/keywords.gperf"
    {"__atomic_and_fetch", ID___ATOMIC_AND_FETCH, KW_ATOMIC},
#line 22 "src/keywords.gperf"
    {"default", ID_DEFAULT, KW_CONTROL},
#line 169 "src/keywords.gperf"
    {"__builtin_choose_expr", ID___BUILTIN_CHOOSE_EXPR, KW_BUILTIN},
#line 64 "src/keywords.gperf"
    {"_Float128", ID__FLOAT128, KW_TYPE},
    {(char*)0},
#line 256 "src/keywords.gperf"
    {"__sync_lock_release", ID___SYNC_LOCK_RELEASE, KW_SYNC},
#line 203 "src/keywords.gperf"
    {"__builtin_popcount", ID___BUILTIN_POPCOUNT, KW_BUILTIN},
#line 252 "src/keywords.gperf"
    {"__sync_fetch_and_nand", ID___SYNC_FETCH_AND_NAND, KW_SYNC},
#line 32 "src/keywords.gperf"
    {"__inline__", ID___INLINE__, KW_STORAGE},
#line 156 "src/keywords.gperf"
    {"memset", ID_MEMSET, KW_BUILTIN},
#line 125 "src/keywords.gperf"
    {"define", ID_DEFINE, KW_PREPROC},
#line 140 "src/keywords.gperf"
    {"defined", ID_DEFINED, KW_PREPROC},
#line 157 "src/keywords.gperf"
    {"memcpy", ID_MEMCPY, KW_BUILTIN},
    {(char*)0},
#line 180 "src/keywords.gperf"
    {"__builtin_copysignl", ID___BUILTIN_COPYSIGNL, KW_BUILTIN},
#line 163 "src/keywords.gperf"
    {"__builtin_add_overflow", ID___BUILTIN_ADD_OVERFLOW, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 179 "src/keywords.gperf"
    {"__builtin_copysignf", ID___BUILTIN_COPYSIGNF, KW_BUILTIN},
    {(char*)0},
#line 62 "src/keywords.gperf"
    {"_Float64", ID__FLOAT64, KW_TYPE},
#line 99 "src/keywords.gperf"
    {"__fastcall", ID___FASTCALL, KW_ASM},
#line 241 "src/keywords.gperf"
    {"__atomic_or_fetch", ID___ATOMIC_OR_FETCH, KW_ATOMIC},
    {(char*)0},
#line 144 "src/keywords.gperf"
    {"alloca", ID_ALLOCA, KW_BUILTIN},
#line 53 "src/keywords.gperf"
    {"__int64", ID___INT64, KW_TYPE},
#line 40 "src/keywords.gperf"
    {"volatile", ID_VOLATILE, KW_QUAL},
#line 194 "src/keywords.gperf"
    {"__builtin_memcmp", ID___BUILTIN_MEMCMP, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 80 "src/keywords.gperf"
    {"alias", ID_ALIAS, KW_ATTR},
#line 136 "src/keywords.gperf"
    {"pop", ID_POP, KW_PREPROC},
    {(char*)0},
#line 263 "src/keywords.gperf"
    {"__vfprintf_chk", ID___VFPRINTF_CHK, KW_BUILTIN},
    {(char*)0},
#line 251 "src/keywords.gperf"
    {"__sync_fetch_and_and", ID___SYNC_FETCH_AND_AND, KW_SYNC},
#line 31 "src/keywords.gperf"
    {"__inline", ID___INLINE, KW_STORAGE},
#line 63 "src/keywords.gperf"
    {"_Float64x", ID__FLOAT64X, KW_TYPE},
    {(char*)0},
#line 145 "src/keywords.gperf"
    {"printf", ID_PRINTF, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 26 "src/keywords.gperf"
    {"goto", ID_GOTO, KW_CONTROL},
    {(char*)0}, {(char*)0},
#line 138 "src/keywords.gperf"
    {"pop_macro", ID_POP_MACRO, KW_PREPROC},
    {(char*)0}, {(char*)0},
#line 219 "src/keywords.gperf"
    {"__builtin_va_arg_pack", ID___BUILTIN_VA_ARG_PACK, KW_SPECIAL},
    {(char*)0},
#line 121 "src/keywords.gperf"
    {"asm", ID_ASM, KW_SPECIAL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 247 "src/keywords.gperf"
    {"__atomic_thread_fence", ID___ATOMIC_THREAD_FENCE, KW_ATOMIC},
#line 49 "src/keywords.gperf"
    {"void", ID_VOID, KW_TYPE},
    {(char*)0},
#line 139 "src/keywords.gperf"
    {"unicode", ID_UNICODE, KW_PREPROC},
    {(char*)0}, {(char*)0},
#line 261 "src/keywords.gperf"
    {"__vprintf_chk", ID___VPRINTF_CHK, KW_BUILTIN},
    {(char*)0},
#line 82 "src/keywords.gperf"
    {"aligned", ID_ALIGNED, KW_ATTR},
#line 217 "src/keywords.gperf"
    {"__builtin_types_compatible_p", ID___BUILTIN_TYPES_COMPATIBLE_P, KW_BUILTIN},
#line 117 "src/keywords.gperf"
    {"__has_include_next", ID___HAS_INCLUDE_NEXT, KW_SPECIAL},
#line 158 "src/keywords.gperf"
    {"memcmp", ID_MEMCMP, KW_BUILTIN},
    {(char*)0}, {(char*)0},
#line 20 "src/keywords.gperf"
    {"switch", ID_SWITCH, KW_CONTROL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 90 "src/keywords.gperf"
    {"gcc_struct", ID_GCC_STRUCT, KW_ATTR},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 230 "src/keywords.gperf"
    {"__atomic_exchange_n", ID___ATOMIC_EXCHANGE_N, KW_ATOMIC},
    {(char*)0},
#line 220 "src/keywords.gperf"
    {"__builtin_va_arg_pack_len", ID___BUILTIN_VA_ARG_PACK_LEN, KW_SPECIAL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 61 "src/keywords.gperf"
    {"_Float32x", ID__FLOAT32X, KW_TYPE},
    {(char*)0},
#line 227 "src/keywords.gperf"
    {"__atomic_compare_exchange", ID___ATOMIC_COMPARE_EXCHANGE, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 17 "src/keywords.gperf"
    {"while", ID_WHILE, KW_CONTROL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 218 "src/keywords.gperf"
    {"__builtin_va_arg", ID___BUILTIN_VA_ARG, KW_SPECIAL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 198 "src/keywords.gperf"
    {"__builtin_mul_overflow_p", ID___BUILTIN_MUL_OVERFLOW_P, KW_BUILTIN},
    {(char*)0},
#line 178 "src/keywords.gperf"
    {"__builtin_copysign", ID___BUILTIN_COPYSIGN, KW_BUILTIN},
#line 60 "src/keywords.gperf"
    {"_Float32", ID__FLOAT32, KW_TYPE},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 242 "src/keywords.gperf"
    {"__atomic_signal_fence", ID___ATOMIC_SIGNAL_FENCE, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 69 "src/keywords.gperf"
    {"union", ID_UNION, KW_TYPE},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 197 "src/keywords.gperf"
    {"__builtin_mul_overflow", ID___BUILTIN_MUL_OVERFLOW, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 260 "src/keywords.gperf"
    {"__printf_chk", ID___PRINTF_CHK, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 228 "src/keywords.gperf"
    {"__atomic_compare_exchange_n", ID___ATOMIC_COMPARE_EXCHANGE_N, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 262 "src/keywords.gperf"
    {"__fprintf_chk", ID___FPRINTF_CHK, KW_BUILTIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 240 "src/keywords.gperf"
    {"__atomic_nand_fetch", ID___ATOMIC_NAND_FETCH, KW_ATOMIC},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 165 "src/keywords.gperf"
    {"__builtin_apply_args", ID___BUILTIN_APPLY_ARGS, KW_BUILTIN}
  };

const struct keyword_entry *
keyword_lookup (str, len)
     const char *str;
     size_t len;
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            const char *s = keyword_table[key].name;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &keyword_table[key];
          }
    }
  return 0;
}
