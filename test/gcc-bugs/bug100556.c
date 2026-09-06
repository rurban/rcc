/* GCC Bug #100556 - ICE: in gimplify_expr with __transaction_atomic and comparisons (and boolean and/ors)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100556
 */
/* { dg-do compile } */


int f(void);
int f2() { return __transaction_atomic(f() == 0); }
// --- CUT ---
// <source>:2:41: error: unsafe function call 'int f()' within atomic transaction
//     2 | int f2() { return __transaction_atomic(f() == 0); }
//       |                                        ~^~


