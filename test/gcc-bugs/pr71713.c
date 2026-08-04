/* GCC Bug #71713 - "initializer element is not constant" with nested casts
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71713
 */
/* { dg-do compile } */


struct Str1 { int i; };
 struct Str2 { struct Str1 *m; };
// Gcc fails to compile this:
 const struct Str2 cnt = { .m = (struct Str1[]){
  (struct Str1){1}
 }};
// Reporting "error: initializer element is not constant"
// If the inner cast is removed:
 const struct Str2 cnt = { .m = (struct Str1[]){
  {1}
 }};
// Then it works fine.
// As a side note, clang accepts it.
// Thanks,
//   Vicente.


