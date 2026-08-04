/* GCC Bug #96952 - __builtin_thread_pointer support cannot be probed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96952
 */
/* { dg-do compile } */


void *get_tp() { return __builtin_thread_pointer(); }

//     2 | void *get_tp() { return __builtin_thread_pointer(); }


