/* GCC Bug #81233 - -Wdiscarded-qualifiers and Wincompatible-pointer-types missing important detail
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81233
 */


struct S { int *p; char *q; const char *r; };

void f (struct S *s)
{
//   s->p = s->q;
//   s->q = s->r;
}
// x.c: In function ‘f’:
// x.c:5:8: warning: assignment from incompatible pointer type [-Wincompatible-pointer-types]
//    s->p = s->q;
//         ^
// x.c:6:8: warning: assignment discards ‘const’ qualifier from pointer target type [-Wdiscarded-qualifiers]
//    s->q = s->r;
//         ^
// In contrast to GCC, Clang issues the following messages:
// t.c:5:8: warning: incompatible pointer types assigning to 'int *' from 'char *'
//       [-Wincompatible-pointer-types]
//   s->p = s->q;
//        ^ ~~~~
// t.c:6:8: warning: assigning to 'char *' from 'const char *' discards qualifiers
//       [-Wincompatible-pointer-types-discards-qualifiers]
//   s->q = s->r;
//        ^ ~~~~


