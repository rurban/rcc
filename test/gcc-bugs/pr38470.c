/* GCC Bug #38470 - value range propagation (VRP) would improve -Wsign-compare
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38470
 */
/* { dg-do compile } */


extern int
// f(short s, unsigned int u)
{
    return s == u % 100;
}
// It would be nice if the compiler noticed that rhs is always within 0..SHRT_MAX, so the comparison is not surprisingly affected by integer promotion.


