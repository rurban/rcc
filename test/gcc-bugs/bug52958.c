/* GCC Bug #52958 - Missing warning on missed parehthesis
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52958
 */
/* { dg-do compile } */

#include <stdbool.h>

int
test (bool b, int x, int y)
{
  return 42 + b ? x : y;
}

// clang gives (for the equivalent C++ testcase "parentheses4.cpp"):
//
// parentheses4.cpp:2:17: warning: operator '?:' has lower precedence than
// '+'; '+' will be evaluated first [-Wparentheses]
//  return 42 + b ? x : y;
//          ~~~~~~ ^
// parentheses4.cpp:2:17: note: place parentheses around the '+' expression
// to silence this warning
//  return 42 + b ? x : y;
//                 ^
//          (     )
// parentheses4.cpp:2:17: note: place parentheses around the '?:' expression
// to evaluate it first
//  return 42 + b ? x : y;
//                 ^
//               (        )
// 1 warning generated.
//
// GCC gives no such warning, even with -Wall -Wparentheses; still
// unimplemented as of this writing, hence no dg-warning here.
