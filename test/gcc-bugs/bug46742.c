/* GCC Bug #46742 - -Wparentheses unexpectedly misses some cases [-Wbool-bitwise-parentheses]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46742
 */
/* { dg-do compile } */


int foo(unsigned int mask) { return (!mask & 2) ? 1 : 0; }
int bar(unsigned int mask) { return (!mask & 1) ? 1 : 0; }


