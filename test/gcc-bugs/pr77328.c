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
// t.c: In function ‘f’:
// t.c:7:30: warning: format ‘%i’ expects argument of type ‘int’, but argument 4 has type ‘double’ [-Wformat=]
   __builtin_sprintf (d, "%i %i", 1, 2.0);
//                              ~^
//                              %f
// t.c:9:9: warning: format ‘%i’ expects argument of type ‘int’, but argument 4 has type ‘double’ [-Wformat=]
   P (d, "%i %i", 1, 2.0);
//          ^
// t.c:5:45: note: in definition of macro ‘P’
 #define P(d, f, a, b) __builtin_sprintf (d, f, a, b)
//                                              ^


