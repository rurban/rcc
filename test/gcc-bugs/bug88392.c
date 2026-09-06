/* GCC Bug #88392 - Incorrect error message when using bitfields in void *pointer
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88392
 */
/* { dg-do compile } */

struct {
   void    *v:44; /* { dg-error "bit-field .v. has invalid type" } */
} z;
