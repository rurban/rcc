/* GCC Bug #77331 - incorrect range location in -Wformat with a concatenated format literal
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77331
 */
/* { dg-do compile } */


extern int printf (const char*, ...);

void f (const char *msg)
{
  printf ("hello " "%i", msg);

#define INT_FMT "%i"

  printf ("hello " INT_FMT " world", msg);

}

