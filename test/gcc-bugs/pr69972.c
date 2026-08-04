/* GCC Bug #69972 - duplicate integer overflow diagnostic in constant expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69972
 */


enum { e = __INT_MAX__ + 1 };
//                         ^
// x.c:1:8: warning: overflow in constant expression [-Woverflow]
 enum { e = __INT_MAX__ + 1 };
//         ^
// G++ issues two similar kinds of diagnostics for the following code, one -Woverflow and another -fpermissive:
template <int N> struct S { };
// S<(__INT_MAX__ + 1)> s;
// t.c:2:16: warning: integer overflow in expression [-Woverflow]
//  S<(__INT_MAX__ + 1)> s;
//     ~~~~~~~~~~~~^~~
// t.c:2:20: error: overflow in constant expression [-fpermissive]
//  S<(__INT_MAX__ + 1)> s;
//                     ^
// t.c:2:20: note: in template argument for type ‘int’


