/* GCC Bug #45780 - Warning for arithmetic operations involving C99 _Bool variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45780
 */
/* { dg-do compile } */
/* { dg-options "-Wall -Wbool-operation" } */


#include <stdbool.h>
#include <stddef.h>

static bool f1(int i)
{
  return i;
}

static bool f2(int i)
{
//   i = !!i;
  return i;
}

int main(int argc, char **argv)
{
  bool a = true;
  bool b = a++;
  bool c = ++b;
  bool d = argc;
  bool e = a & argc;
  bool f = b | argc;
  bool g = c ^ argc;
  f &= argc;
  g |= argc;
  e ^= argc;
  if (f1(argc))
//     e--;
//   else
//     --e;
  if (!!argc)
    return ((argv != NULL) ? d : ((f > g) ? e : (f << g)));
//   else
    return f2(argc);
}
// $
// gcc's -Wbool-operation currently catches the increments and decrements, but none of the rest of the operations:
// 45780.c: In function 'main':
// 45780.c:18:13: warning: increment of a boolean expression [-Wbool-operation]
//    18 |   bool b = a++;
//       |             ^~
// 45780.c:19:12: warning: increment of a boolean expression [-Wbool-operation]
//    19 |   bool c = ++b;
//       |            ^~
// 45780.c:28:6: warning: decrement of a boolean expression [-Wbool-operation]
//    28 |     e--;
//       |      ^~
// 45780.c:30:5: warning: decrement of a boolean expression [-Wbool-operation]
//    30 |     --e;
//       |     ^~
// $


