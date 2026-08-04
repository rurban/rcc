/* GCC Bug #111269 - location for non-constant expressions inside static assert could be better
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111269
 */


{
 int x = 42;

// 	_Static_assert(0 || 7 > x, "");
}
// const.c: In function ‘main’:
// const.c:6:26: error: expression in static assertion is not constant
//     6 |         _Static_assert(0 || 7 > x, "");
//       |                        ~~^~~~~~~~
// const.c:6:26: error: static assertion expression is not an integral constant expression
//         _Static_assert(0 || 7 > x, "");
//                        ~~~~~~~~~^
// 1 error generated.
// Clang points to the precise location of the problem, while GCC is too fuzzy.
// I suspect this is a duplicate of other bugs, but I'm not sure, so I'll let you decide that.


