/* GCC Bug #117291 - Simple but large test case uses up over 8M of stack and hits SEGV
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117291
 */


{
#if defined(HAVE_SETRLIMIT) && defined(HAVE_GETRLIMIT) \
//     && defined(RLIMIT_STACK) && defined(RLIM_INFINITY)
// ...
// I'll have to see why one or more of these are not set on Power.


