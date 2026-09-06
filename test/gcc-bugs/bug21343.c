/* GCC Bug #21343 - incompatible internal linkage declarations in different scopes not diagnosed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21343
 */
/* { dg-do compile } */


static int (*a)[];
void f(void) { extern int (*a)[2]; }
void g(void) { extern int (*a)[3]; }



