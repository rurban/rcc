/* GCC Bug #108954 - ICE with invalid gimple source
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108954
 */
/* { dg-do compile } */


typedef int v4si __attribute__((vector_size(16)));

int __GIMPLE (ssa,startwith("fre"))
// foo (int c)
{
  int * p;
  int i;
  int x[4];
  short unsigned int _1;
  short unsigned int _2;
  int _7;
  v4si _6;

//   __BB(2):
  i_3 = 0;
  _1 = (short unsigned int) i_3;
  _2 = _1 * 4ul;
  p_4 = _Literal (int *) &x + _2; /* { dg-error "" } */
  _6 = _Literal (v4si) { c_5(D), c_5(D), c_5(D), c_5(D) };
  __MEM <v4si> ((v4si *)p_4) = _6;
  _7 = x[0];
  return _7;
}

//    19 |   p_4 = _Literal (int *) &x + _2;


