/* GCC Bug #59520 - a possible inconsistency in error diagnostics with "-pedantic -std=c99"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59520
 */


> {
> };
> {
> } d;
> {
>   struct S0 g = {0,0};
// as if you did struct S0 g = {0,0}; memcpy (&d.f0, &g, sizeof (int)); printf ("%d\n", d.f0);


