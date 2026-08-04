/* GCC Bug #70477 - -Wtautological-compare too aggressive?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70477
 */
/* { dg-do compile } */


if (BYTES_BIG_ENDIAN != WORDS_BIG_ENDIAN)
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                    ^ 
// For aarch64 these are defined as:
#define BYTES_BIG_ENDIAN (TARGET_BIG_END != 0)
#define WORDS_BIG_ENDIAN (BYTES_BIG_ENDIAN)
// I think this can be reproduced also as:
int var;

#define A var
#define B var

int foo (int a, int b)
{
  if (A == B)
    return a;
//   else
    return b;
}
// warn.c: In function 'foo':
// warn.c:8:9: warning: self-comparison always evaluates to true [-Wtautological-compare]
   if (A == B)
//          ^~
// Should we be warning on these things if they come from a macro expansion?


