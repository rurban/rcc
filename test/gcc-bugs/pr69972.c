/* GCC Bug #69972 - duplicate integer overflow diagnostic in constant expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69972
 */
/* { dg-do compile } */


enum { e = __INT_MAX__ + 1 };

