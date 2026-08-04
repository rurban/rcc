/* GCC Bug #69602 - over-ambitious logical-op warning on EAGAIN vs EWOULDBLOCK
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69602
 */


extern int xxx;
#define XXX xxx
int test (void)
{
  if (!XXX && xxx)
    return 4;
//   else
    return 0;
}
// thus, if this is not a regression caused by something else, it is a duplicate.


