/* GCC Bug #81141 - missing warning using sizeof a/sizeof *a with a zero-length array
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81141
 */
/* { dg-do compile } */


struct S { char n, *s, a[0]; };

void h (void *d, const struct S *s)
{
  __builtin_memcpy (d, s->s, sizeof s->s / sizeof *s->s);   // warning (good)
  __builtin_memcpy (d, s->a, sizeof s->a / sizeof *s->a);   // missing warning

  extern char a[0];

  __builtin_memcpy (d, s->a, sizeof a / sizeof *a);         // missing warning
}
// x.c: In function ‘h’:
// x.c:5:42: warning: division ‘sizeof (char * const) / sizeof (char)’ does not compute the number of array elements [-Wsizeof-pointer-div]
// __builtin_memcpy (d, s->s, sizeof s->s / sizeof *s->s);   // warning (good)
//                                           ^


