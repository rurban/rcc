/* GCC Bug #104171 - ICE: 'verify_gimple' failed. verify_gimple_in_seq; gimplify_body
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104171
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu89 -fchecking" } */

/* Reporter's t.c: unprototyped cimagl() builtin call with an unpromoted
 * _Complex argument folds to IMAGPART_EXPR with a mismatched type,
 * causing 'verify_gimple' failed ICE on checking-enabled builds. */
long double cimagl();
test_cimagl(_Complex z) { return cimagl(z); }