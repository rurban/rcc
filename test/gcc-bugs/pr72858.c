/* GCC Bug #72858 - incorrect fixit hints in -Wformat diagnostics
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=72858
 */
/* { dg-do compile } */


int f (char *d, long x)
{
  extern int sprintf (char*, const char*, ...);
  sprintf (d, "%-8x", x);
}
// xyz.c: In function ‘f’:
// xyz.c:4:19: warning: format ‘%x’ expects argument of type ‘unsigned int’, but argument 3 has type ‘long int’ [-Wformat=]
   sprintf (d, "%-8x", x);
//                 ~~~^
//                 %ld
// Clang provides the expected hint:
// xyz.c:4:23: warning: format specifies type 'unsigned int' but the argument has
//       type 'long' [-Wformat]
  sprintf (d, "%-8x", x);
//                ~~~~   ^
//                %-8lx


