/* GCC Bug #71603 - pragma diagnostic pop fails to restore warning level
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71603
 */
/* { dg-do compile } */


void f (const char *s)
{
#pragma GCC diagnostic warning "-Wformat=1"

  __builtin_printf (s, s);   // no -Wformat diagnostic expected

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wformat=2"
  __builtin_printf (s, s);   // -Wformat-nonliteral expected
#pragma GCC diagnostic pop

  __builtin_printf (s, s);   // no diagnostic expected
}

int x;

int g (void)
{
  enum { i = 1 };

#pragma GCC diagnostic warning "-Wshift-overflow=1"

  x = i << 31;   // no -Wshift-overflow expected

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wshift-overflow=2"

  x = i << 31;   // -Wshift-overflow expected

#pragma GCC diagnostic pop
  x = i << 31;   // no warning expected
}
// t.C: In function ‘f’:
// t.C:9:3: warning: format not a string literal, argument types not checked [-Wformat-nonliteral]
//    __builtin_printf (s, s);   // -Wformat-nonliteral expected
//    ^~~~~~~~~~~~~~~~
// t.C:12:3: warning: format not a string literal, argument types not checked [-Wformat-nonliteral]
//    __builtin_printf (s, s);   // no diagnostic expected
//    ^~~~~~~~~~~~~~~~
// t.C: In function ‘g’:
// t.C:28:9: warning: result of ‘1 << 31’ requires 33 bits to represent, but ‘int’ only has 32 bits [-Wshift-overflow=]
//    x = i << 31;   // -Wshift-overflow expected
//          ^~
// t.C:31:9: warning: result of ‘1 << 31’ requires 33 bits to represent, but ‘int’ only has 32 bits [-Wshift-overflow=]
//    x = i << 31;   // no warning expected
//          ^~


