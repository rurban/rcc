/* GCC Bug #80745 - inconsistent warning: large integer implicitly truncated to unsigned type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80745
 */
/* { dg-do compile } */


#include <limits.h>

unsigned char uc1 = UCHAR_MAX + 1U;
unsigned char uc2 = USHRT_MAX + 1U;
unsigned char uc3 = UINT_MAX + 1U;
unsigned char uc4 = ULONG_MAX + 1LU;

// t.c:3:21: warning: large integer implicitly truncated to unsigned type [-Woverflow]
 unsigned char uc1 = UCHAR_MAX + 1U;
//                      ^~~~~~~~~
// t.c:4:21: warning: large integer implicitly truncated to unsigned type [-Woverflow]
 unsigned char uc2 = USHRT_MAX + 1U;
//                      ^~~~~~~~~


