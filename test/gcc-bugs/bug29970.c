/* GCC Bug #29970 - mixing ({...}) with VLA leads to massive breakage
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=29970
 */
/* { dg-do compile } */


int foo(int n)  // should not ICE
{
        return ({struct {int x[n];} x; x.x[12] = 1; x;}).x[12];
}
// A sane approach would be to require the type of ({...}) to make sense
// on the outside.  AFAICS, the root cause of that crap is that ({...})
// allows leaking types out of scope where they are defined...
//
// In any case, ICE even on violated constraints is Not Nice(tm),
// especially when those constraints are never stated.
//
// The report includes several further variants that trigger ICEs or wrong
// code with statement expressions ({...}) combined with variably modified
// (VLA) types; kept here for reference only (not compiled):
//
// testcase 2:
// int foo(void)   // should not ICE
// {
//         return sizeof({int n = 20; struct {int x[n];} x; x.x[12] = 1; x;});
// }
// testcase 3:
// int foo(void)   // should not return 0
// {
//         int n = 0;
//         return sizeof({n = 10; struct {int x[n];} x; x;});
// }
// testcase 4:
// int foo(void)   // should not ICE
// {
//         return (*({
//                         int n = 20;
//                         char (*x)[n][n] = malloc(n * n);
//                         (*x)[12][1] = 1;
//                         x;
//                 }))[12][1];
// }
// testcase 5:
// int foo(void)   // should return 1, returns 0
// {
//         int n = 0;
//         return (*({
//                         n = 20;
//                         char (*x)[n][n] = malloc(n * n);
//                         (*x)[12][1] = 1;
//                         (*x)[0][1] = 0;
//                         x;
//                 }))[12][1];
// }
