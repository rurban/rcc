/* GCC Bug #105156 - No diagnostic for `enum { toobig = UINT_MAX }`
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105156
 */


correctly diagnose "enum { toobig = 0x7fffffff * 2U + 1U };".
     2  enum { toobig    = (0x7fffffff * 2U + 1U) };
     3  enum { toobigtoo = UINT_MAX };
//     2 | enum { toobig    = (0x7fffffff * 2U + 1U) };
enum { toobig = (0x7fffffff * 2U + 1U) };
enum { toobigtoo =
# 3 "c.c" 3 4
//                   (0x7fffffff * 2U + 1U)
# 3 "c.c"
                           };
// $


