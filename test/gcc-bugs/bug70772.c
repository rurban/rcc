/* GCC Bug #70772 - Wrong warning about unspecified behavior for comparison with string literal
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70772
 */
/* { dg-do compile } */


void f (void)
{
  const __PTRDIFF_TYPE__ a[] = {
//     "a" == "b",   // no warning expected (behavior well defined)
//     "a" != "b",   // no warning expected (behavior well defined)
//     "a" <  "b",   // warning expected in C only (undefined behavior)
//     "a" <= "b",   // warning expected in C only (undefined behavior)
//     "a" >  "b",   // warning expected in C only (undefined behavior)
//     "a" >= "b",   // warning expected in C only (undefined behavior)
//     "a" -  "b"    // warning expected (undefined behavior)
  };
//   (void)a;
}
// v.c: In function ‘f’:
// v.c:4:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" == "b",   // no warning expected (behavior well defined)
//          ^~
// v.c:5:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" != "b",   // no warning expected (behavior well defined)
//          ^~
// v.c:6:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" <  "b",   // warning expected in C only (undefined behavior)
//          ^
// v.c:7:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" <= "b",   // warning expected in C only (undefined behavior)
//          ^~
// v.c:8:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" >  "b",   // warning expected in C only (undefined behavior)
//          ^
// v.c:9:9: warning: comparison with string literal results in unspecified behavior [-Waddress]
//      "a" >= "b",   // warning expected in C only (undefined behavior)
//          ^~


