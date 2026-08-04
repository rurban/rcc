/* GCC Bug #56528 - __attribute__((visibility)) ignored for a function declaration with an asm label
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56528
 */
/* { dg-do compile } */


// Adding an asm label attribute to a function declaration with a visibility attribute causes the visibility attribute to be ignored:
$ echo 'void f() __attribute__((visibility("hidden"))); void g() { f(); }' | gcc -x c - -S -o - | grep hidden
//         .hidden f
$ echo 'void f() __asm__("f") __attribute__((visibility("hidden"))); void g() { f(); }' | gcc -x c - -S -o - | grep hidden
// $


