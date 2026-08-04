/* GCC Bug #81453 - relational expression involving null pointer not diagnosed with -Wall
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81453
 */
/* { dg-do compile } */


// /* compile with -std=c23 -Wextra */
#include <stddef.h>

int main(void)
{
    void *voidp0=0;

    if (nullptr < voidp0)   /* invalid for clang, valid for gcc nullptr seen as integer 0 */
//     if (nullptr < nullptr)  /* but ordered compare cannot have nullptr */
     return 1;
}
// ```
// gives
// <source>: In function 'main':
// <source>:8:17: warning: ordered comparison of pointer with integer zero [-Wextra]
//     8 |     if (nullptr < voidp0)   /* invalid for clang, valid for gcc nullptr seen as integer 0 */
//       |                 ^
// <source>:9:17: error: invalid operands to binary < (have 'typeof (nullptr)' and 'typeof (nullptr)')
//     9 |     if (nullptr < nullptr)  /* but ordered compare cannot have nullptr */
//       |                 ^
// Compiler returned: 1
// Thanks!


