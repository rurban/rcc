/* GCC Bug #123466 - ICE in get_unwidened after redeclared error
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123466
 */
/* { dg-do compile } */


unsigned ll = 0 ;
void  foo ( void ) {
//   1 << ( ll - 1ll );
}
void ll () {}
// ```


