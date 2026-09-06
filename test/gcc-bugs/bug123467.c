/* GCC Bug #123467 - ICE in force_constant_size with VLA initialized in function type definition
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123467
 */
/* { dg-do compile } */


int x;
void g(int b[(int[x]){1}[0]]) {
}
// ```


