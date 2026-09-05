/* GCC Bug #72858 - incorrect fixit hints in -Wformat diagnostics
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=72858
 */
/* { dg-do compile } */


int f (char *d, long x)
{
  extern int sprintf (char*, const char*, ...);
  sprintf (d, "%-8x", x);
}

