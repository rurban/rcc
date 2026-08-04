/* GCC Bug #108395 - [C2x] Bogus -Wunused-but-set-variable when returning constexpr variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108395
 */
/* { dg-do compile } */


# echo 'int f() { constexpr int v = 0; return v; }' | gcc -c -xc -std=c2x -Wall -Wextra -


