/* GCC Bug #88392 - Incorrect error message when using bitfields in void *pointer
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88392
 */
/* { dg-do compile } */


struct {
   void    *v:44;
} z;
// b.c:2:13: error: bit-field ‘v’ has invalid type
    void    *v:44;
//              ^
// b.c:2:13: error: width of ‘v’ exceeds its type
// The first message is fine, but the second is I believe erroneous as sizeof (void *) is 8 on that machine. A bit misleading but not a big deal, though.
// All the best,
// Frédéric


