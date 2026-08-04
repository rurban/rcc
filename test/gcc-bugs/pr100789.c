/* GCC Bug #100789 - ICE with __transaction_relaxed and left shift signed overflow
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100789
 */
/* { dg-do compile } */


{
//   <<< Unknown tree: c_maybe_const_expr
//     8526495038820057088 >>>
}
// I don't know if it's OK to fold the insides of a TRANSATION_EXPR.  Another option would to avoid creating C_MAYBE_CONST_EXPRs inside a transaction expr, like in_late_binary_op?


