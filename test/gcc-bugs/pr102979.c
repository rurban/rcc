/* GCC Bug #102979 - GCC gives wrong error for struct definitions without semicolon, despite G++ already giving correct one
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102979
 */
/* { dg-do compile } */


struct test{int i;}
// ```
// <a href="https://godbolt.org/z/GEz56T7bT">https://godbolt.org/z/GEz56T7bT</a>
// Current error: "error: expected identifier or '(' at end of input" (points to "test")
// G++ on the other hand gives reasonable:
// "error: expected ';' after struct definition"
// <a href="https://godbolt.org/z/YGqvY7bsa">https://godbolt.org/z/YGqvY7bsa</a>
// Is it possible to get G++'s error in GCC too?


