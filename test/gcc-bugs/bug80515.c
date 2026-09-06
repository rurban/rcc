/* GCC Bug #80515 - __attribute__ ((__noreturn__)) false alarm for 'main'
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80515
 */
/* { dg-do compile } */

/* Enhancement request: main() whose body is an infinite loop should not
 * need __attribute__((noreturn)) (clang/icc accept it; gcc warns).
 * Compile-only - the loop never terminates at runtime. */
int main() {
   for (;;)
     ;
   return 0;
 }
