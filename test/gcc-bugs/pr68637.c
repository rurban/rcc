/* GCC Bug #68637 - Array of function pointers with attribute leads to wrong code
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68637
 */
/* { dg-do compile } */

typedef void (*func) (int, int) __attribute__ ((regparm (2)));
extern func foo[10];
extern void (*bar[10]) (int, int) __attribute__ ((regparm (2)));

void
xxx (int i)
{
  foo[i] (1, 2);
  bar[i] (1, 2);
}
