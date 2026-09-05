/* GCC Bug #84500 - diagnostic says "array of chars" for arrays of wchar_t, char16_t and char32_t
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84500
 */


#include <stddef.h>
#include <uchar.h>
int main() {
  wchar_t w[3] = L"abcd";
  char16_t u16[3] = u"abcd";
  char32_t u32[3] = U"abcd";
}
// a.c: In function ‘main’:
// a.c:4:18: warning: initializer-string for array of chars is too long
   wchar_t w[3] = L"abcd";
//                   ^~~~~~~
// a.c:5:21: warning: initializer-string for array of chars is too long
   char16_t u16[3] = u"abcd";
//                      ^~~~~~~
// a.c:6:21: warning: initializer-string for array of chars is too long
   char32_t u32[3] = U"abcd";
//                      ^~~~~~~
// Also:
// #include <stddef.h>
// int main() {
//   char c[3] = "abc";
//   wchar_t w[3] = L"abc";
// }
// a.c: In function 'main':
// a.c:4:15: warning: initializer-string for array chars is too long for C++ [-Wc++-compat]
//    char c[3] = "abc";
//                 ^~~~~
// a.c:5:18: warning: initializer-string for array chars is too long for C++ [-Wc++-compat]
//    wchar_t w[3] = L"abc";
// Note "array chars" not "array of chars".


