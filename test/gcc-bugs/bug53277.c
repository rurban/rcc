/* GCC Bug #53277 - -Wconversion cannot handle compound expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53277
 */
/* { dg-options "-Wconversion" } */

// The original report used glibc's strspn() macro:
//
//   #  define strspn(s, accept) \
//     __extension__                                                       \
//     ({ char __a0, __a1, __a2;                                           \
//        (__builtin_constant_p (accept) && __string2_1bptr_p (accept)     \
//         ? ((__builtin_constant_p (s) && __string2_1bptr_p (s))          \
//            ? __builtin_strspn (s, accept)                               \
//            : ((__a0 = ((const char *) (accept))[0], __a0 == '\0')       \
//               ? ((void) (s), 0)                                        \
//               : ...))                                                  \
//         : __builtin_strspn (s, accept)); })
//
// whose expansion contains a conditional expression with a lone
// constant 0 that -Wconversion fails to recognize as constant because
// it is produced by a comma expression such as "((void) s, 0)".
// Reduced testcase from comment 15:
int main (void)
{
  char i = 1;
  char x = ((void) i, 0);
  x = i ? x : ((void) i, 0); /* { dg-warning "conversion" } */
  return 0;
}
