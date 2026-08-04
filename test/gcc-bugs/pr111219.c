/* GCC Bug #111219 - -Wformat-truncation intentional false negative with %p modifier is undocumented
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111219
 */
/* { dg-do compile } */


void foo (void *x) {
    char dst [1];
    __builtin_snprintf(dst, sizeof(dst), "%p", x);
}
// ```
// Clang-18 (trunk, not yet released, after <a href="https://github.com/llvm/llvm-project/commit/0c9c9dd9a24f9d715d950fef0ac7aae01437af96">https://github.com/llvm/llvm-project/commit/0c9c9dd9a24f9d715d950fef0ac7aae01437af96</a>) with -Wfortify-source will warn:
// ```
// tmp.c:3:5: warning: 'snprintf' will always be truncated; specified size is 1, but format string expands to at least 4 [-Wfortify-source]
//     3 |     __builtin_snprintf(dst, sizeof(dst), "%p", x);
//       |     ^
// ```
// GCC with -Wformat-truncation does not warn, but I think it should.


