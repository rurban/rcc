/* GCC Bug #114541 - Invalid gimple __BB# accepted due to usage of atoi
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114541
 */
/* { dg-do compile } */


void __GIMPLE (ssa,startwith ("dse2")) foo ()
{
  int a;

// __BB(2):
  if (a_5(D) > 4)
    goto __BB4294967299;
//   else
    goto __BB4;

// __BB(3):
  a_2 = 10;
  goto __BB5;

// __BB(4):
  a_3 = 20;
  goto __BB5;

// __BB(5):
  a_1 = __PHI (__BB3: a_2, __BB4: a_3);
  a_4 = a_1 + 4;

// return;
}
// ```
// This is invalid gimple but the use of atoi in c_parser_gimple_parse_bb_spec (c/gimple-parser.cc) allows to accept it.

// Note also causes ICE if you put any invalid character after the number due to use of BB 1 at that point (I think).


