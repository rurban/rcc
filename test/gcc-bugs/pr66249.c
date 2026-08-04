/* GCC Bug #66249 - -Wformat-signedness should not warn on enums
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66249
 */


#include <stdio.h>
int main(void) {
  int i = 0;
  unsigned u = 0;
  enum e_signed { as, bs = -1 } es = as;
  enum e_unsigned { au, bu = 0xffffffff } eu = au;
  enum e_implementation { ai } ei = ai;
  printf ("%u %d %u %d %u %d %u %d\n", i, u, es, es, eu, eu, ei, ei);
  return 0;
}
// foo.c: In function ‘main’:
// foo.c:8:11: warning: format ‘%u’ expects argument of type ‘unsigned int’, but argument 2 has type ‘int’ [-Wformat=]
   printf ("%u %d %u %d %u %d %u %d\n", i, u, es, es, eu, eu, ei, ei);
//            ^
// foo.c:8:11: warning: format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘unsigned int’ [-Wformat=]
// foo.c:8:11: warning: format ‘%u’ expects argument of type ‘unsigned int’, but argument 4 has type ‘int’ [-Wformat=]
// foo.c:8:11: warning: format ‘%d’ expects argument of type ‘int’, but argument 7 has type ‘unsigned int’ [-Wformat=]
// foo.c:8:11: warning: format ‘%d’ expects argument of type ‘int’, but argument 9 has type ‘unsigned int’ [-Wformat=]
// [dummy@rawhide64 ~]$ 

// The warning for arguments 2, 3, 4, and 7 are expected (2 and 3 to show the normal usage of the warning, 4/5 to show that I can force an enum to be signed by including negative members, 6/7 to show that I can force an enum to be unsigned by including members larger than INT_MAX); and the lack of warning for arguments 5 and 6 is good.  I can squelch the warning for the first four problematic arguments by either using the correct %d vs. %u counterpart, or by changing the signedness of the variable that I'm printing.

// However, argument 8/9 is problematic.  From the compiler warning on argument 9, it looks like gcc defaults to unsigned, at least on my platform.  But I _cannot_ know whether the compiler will pick a signed or unsigned representation for the enum on other platforms (since all members would fit in either type, and since the platform ABI may demand a particular signedness), so I cannot know whether %u or %d would trigger a warning because I picked the wrong type.  My only recourse is to add a cast (such as "%d",(int)ei); but that is ugly.
// And look what happens when I add -fshort-enums to the mix:
// foo.c: In function ‘main’:
// foo.c:8:11: warning: format ‘%u’ expects argument of type ‘unsigned int’, but argument 2 has type ‘int’ [-Wformat=]
   printf ("%u %d %u %d %u %d %u %d\n", i, u, es, es, eu, eu, ei, ei);
//            ^
// foo.c:8:11: warning: format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘unsigned int’ [-Wformat=]
// foo.c:8:11: warning: format ‘%u’ expects argument of type ‘unsigned int’, but argument 4 has type ‘int’ [-Wformat=]
// foo.c:8:11: warning: format ‘%d’ expects argument of type ‘int’, but argument 7 has type ‘unsigned int’ [-Wformat=]
// [dummy@rawhide64 ~]$ 

// Here, arguments 4/5 are still signed, arguments 6/7 are still unsigned; but arguments 8/9 are now a type compatible with 'char', and promotes to 'int' (whether the enum's underlying type is signed or unsigned).  Oddly enough, the compiler did not warn for EITHER 8 or 9, but by the technical argument, "%u" should have warned for argument 8.  And consider what happens from a promotion standpoint: if gcc picked 'char' (rather than 'signed char' or unsigned char') as the underlying type when -fshort-enums is in effect, and my enum contained a value in the range [128-255], then I am now dependent on whether 'char' is signed or unsigned for whether %d will print 128 or -128 (and %u would print 128 or 4294967168).

// Conversely, we should be able to reason that if the compiler was able to pick a smaller type when -fshort-enums is in effect, and particularly if that smaller type is unsigned, then it doesn't matter whether I use %u or %d - every value of the enum will print to the same representation, and therefore, squelching the warning was the right thing to do.  But if that is the case, then omitting -fshort-enums should _also_ squelch the warning - again, if I have an enum that has no members that require the use of 'int' (for an unsigned member) or 'unsigned' (for a member larger than INT_MAX), then we KNOW that the range of the enum is such that both %u and %d will print it identically, regardless of what type is actually assigned to the enum.
// So I'm arguing that -Wformat-signedness has a bug, and should NOT print a warning about a %d vs 'unsigned' mismatch when it is compared against an enum value whose range could be reduced were -fshort-enums were in effect.
// See also <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Improve -Wformat-signedness"
//    href="show_bug.cgi?id=65446">bug 65446</a> where -Wformat-signedness has issues when integer promotion of smaller unsigned types is involved.


