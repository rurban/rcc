/* GCC Bug #87379 - Warn about function pointer casts which differ in variadic-ness [-Wcast-variadic-function-type]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87379
 */
/* { dg-do compile } */

int open (const char *, int, ...);
int (*ptr1) (const char *, int) = (int (*) (const char *, int)) open;
int (*ptr2) (const char *, int, short)
  = (int (*) (const char *, int, short)) open;
void (*ptr3) () = (void (*) ()) open;
void (*ptr4) (double) = (void (*) (double)) open;


