/* GCC Bug #49706 - No warning for (!x > 1) which is always false
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49706
 */


_Bool x;
// ...
if (x > 1)
// or
struct { unsigned int i : 1; } x;
// ...
if (x.i > 1)
// we don't warn either.


