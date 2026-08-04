/* GCC Bug #22249 - GCC does not reject an incompatible type declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=22249
 */


void r(x) int (*x)[2]; {}
void r();
void r(int (*x)[3]);		/* Ideally rejected.  */
// Flags are e.g. -Wall -std=c99 -pedantic


