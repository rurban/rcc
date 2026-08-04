/* GCC Bug #109912 - #pragma GCC diagnostic ignored "-Wall" is ignored
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109912
 */


#pragma GCC diagnostic warning "-Wunused"
#pragma GCC diagnostic ignored "-Wunused"

static int f() {return 0;}
// ```
// Confirmed.


