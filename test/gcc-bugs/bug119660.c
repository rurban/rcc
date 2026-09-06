/* GCC Bug #119660 - builtin functions erroneously suggested as originating in system headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119660
 */
/* { dg-do compile } */

/* From gcc.dg/Wbuiltin-declaration-mismatch-16.c: an invalid declaration
 * of the __builtin_abort builtin.  gcc wrongly adds a note suggesting
 * #include <stdlib.h> for a __builtin_* function (the bug). */
void __builtin_abort (int[foo]); /* { dg-error ".foo. undeclared here .not in a function." } */