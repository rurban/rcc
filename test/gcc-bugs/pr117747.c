/* GCC Bug #117747 - ICE after error with gimple FE and undefined name, negating a constant
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117747
 */
/* { dg-do compile } */


void foo ( ) {
  b = -7 ;
}
// ```


