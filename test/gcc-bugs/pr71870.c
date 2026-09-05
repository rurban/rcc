/* GCC Bug #71870 - wrong location of "%n$" directive in -Wformat
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71870
 */
/* { dg-do compile } */


char d [4];

void f (void)
{
  __builtin_sprintf (d, "%r");

  __builtin_sprintf (d, "%2$i%1$i", 1, 234);
}

