/* GCC Bug #117290 - error: void value not ignored , pointing to the wrong location when ?: is used inside an if
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117290
 */
/* { dg-do compile } */


void f(int t) {
 if (0 ? 0 : __builtin_exit(0))
   ;
}
// ```


