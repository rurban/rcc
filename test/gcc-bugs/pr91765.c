/* GCC Bug #91765 - -Wredundant-decls conflicts with __attribute__((alias))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91765
 */
/* { dg-do compile } */


extern int bar;

static int foo;
extern int bar __attribute__((alias("foo")));
//     6 | extern int bar __attribute__((alias("foo")));
//     2 | extern int bar;
//     6 | int bar __attribute__((alias("foo")));


