/* GCC Bug #71176 - trunk/fixincludes/fixincl.c:162: bad % specifier
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71176
 */
/* { dg-do compile } */


// trunk/fixincludes/fixincl.c:162]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'size_t {aka unsigned long}'.


