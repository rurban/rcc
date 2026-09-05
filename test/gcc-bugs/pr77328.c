/* GCC Bug #77328 - incorrect caret location in -Wformat calling printf via a macro
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77328
 */
/* { dg-do compile } */


void f (void)
{
  char d [8];

#define P(d, f, a, b) __builtin_sprintf (d, f, a, b)

  __builtin_sprintf (d, "%i %i", 1, 2.0);

  P (d, "%i %i", 1, 2.0);
}

