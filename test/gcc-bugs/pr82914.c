/* GCC Bug #82914 - 'struct __attribute__ ((aligned (N))) s' ignores 'aligned' attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82914
 */


struct s { char mem; };

  struct __attribute__ ((foobar))
  s b;
// I view it as a bug.  At a minimum, GCC should point out that it's ignoring the attribute like other compilers do, such as Clang:
//   warning: unknown attribute 'foobar' ignored [-Wunknown-attributes]
// I happened to notice this bug while testing a fix for <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - [8 Regression] incorrect -Wattributes warning for packed/aligned conflict on struct members"
//    href="show_bug.cgi?id=84108">pr84108</a>.  It seems that a simple fix is fairly straightforward so hopefully Richard won't be offended if I reopen this bug, assign it to myself, and submit my patch in stage 1 of GCC 9.


