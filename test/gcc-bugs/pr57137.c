/* GCC Bug #57137 - spurious "format string is not literal" when the format string is marked with __attribute__((format))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57137
 */
/* { dg-do compile } */


void f (const char*, ...) __attribute__ ((format (printf, 1, 2)));

void g (const char*) __attribute__ ((format (printf, 1, 0)));
void g (const char *fmt)
{
  f (fmt, "xxx");
}

void h (const char *fmt)
{
  g (fmt);
}
// v.c: In function 'g':
// v.c:6:3: warning: format not a string literal, argument types not checked [-Wformat-nonliteral]

