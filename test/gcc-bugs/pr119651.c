/* GCC Bug #119651 - internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in tree_nonzero_bits, at fold-const.cc:16702
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119651
 */
/* { dg-do compile } */


int f() {
  int r;
  while (r % 2 == 0) ;
  double r;
}
// ```


