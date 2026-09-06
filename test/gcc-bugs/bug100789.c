/* GCC Bug #100789 - ICE with __transaction_relaxed and left shift signed overflow
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100789
 */
/* { dg-do compile } */
/* { dg-options "-fgnu-tm" } */

/* Original reporter's reproducer (mutant.c):
 * The shift-overflow warning path would create a C_MAYBE_CONST_EXPR inside
 * a TRANSACTION_EXPR, leaking into the gimplifier and causing an ICE
 * in gimplify_expr.  With -fgnu-tm the code is valid (comment 3); modern
 * gcc rejects it with a plain error instead of ICEing. */
int bar(void) { return __transaction_relaxed(0x1234567876543210LL << 32); }


