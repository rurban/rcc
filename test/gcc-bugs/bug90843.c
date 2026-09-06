/* GCC Bug #90843 - pragma diagnostic doesn't affect warnings controlled by extra_warnings and pedantic
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90843
 */
/* { dg-do compile } */


unsigned isalpha (int);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
unsigned isdigit (int);
#pragma GCC diagnostic pop 

// a.c:1:10: warning: mismatch in return type of built-in function ‘isalpha’; expected ‘int’ [-Wbuiltin-declaration-mismatch]
//     1 | unsigned isalpha (int);
// a.c:5:10: warning: mismatch in return type of built-in function ‘isdigit’; expected ‘int’ [-Wbuiltin-declaration-mismatch]
//     5 | unsigned isdigit (int);
#pragma GCC diagnostic warning "-Wpedantic"
int a[0];   // Wpedantic (good)
#pragma GCC diagnostic ignored "-Wpedantic"
int b[0];   // no -Wpedantic (good)

void f (int n)
{
  int a[n];   // -Wvla not suppressed (bug)
//   (void)&a;
}
//     2 | int a[0];   // Wpedantic (good)
// t.C: In function ‘void f(int)’:
//     8 |   int a[n];   // -Wvla not suppressed (bug)


