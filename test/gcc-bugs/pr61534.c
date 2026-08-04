/* GCC Bug #61534 - Wlogical-op should not warn when either operand comes from macro expansion
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61534
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
// The big hurdle is that !XXX becomes XXX == 0, but it has the location of "!", which is not virtual. If we look at the argument of the expression, then XXX is actually a var_decl, whose location corresponds to the declaration and not the use, and it is not virtual either. This is <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - Preserve variable-use locations"
//    href="show_bug.cgi?id=43486">PR43486</a>.


