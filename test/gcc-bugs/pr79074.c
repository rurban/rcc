/* GCC Bug #79074 - -Waddress difference between C and C++ with (T*)0
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79074
 */
/* { dg-do compile } */


int f (int i) { return &i != (void *) 0; }

int g (int i) { return &i != (int *) 0; }

