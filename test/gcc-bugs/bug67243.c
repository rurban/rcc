/* GCC Bug #67243 - Wrong Message of -Wvla for Standard ISO C90 However Emitted with -std=c11
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67243
 */
/* { dg-do compile } */


void square(int num) {
  int array[num];
}
// With -std=c99 -Wpedantic there is no warning.
// With -std=c99 -Wvla, it mentions ISO C90, which is irrelevant.
// Clang just says: "variable length array used"


