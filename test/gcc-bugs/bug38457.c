/* GCC Bug #38457 - -Wattributes gives warnings for portable code for default-packed architectures.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38457
 */
/* { dg-do compile } */


struct x
{
  char c;
  int x  __attribute__ ((__packed__));
};

// *depending on the target ABI*, i.e. it will always warn for a target where the default layout corresponds to the packed layout (example cut down from generic code in glibc-2.9).  Worse, this warning is on by default.

// I know someone will jump up and tell me to remove the attribute in the code, but it doesn't work that way: editing the code is not appropriate (example cut down from generic code in glibc).  Neither is shutting off *all* attribute-warnings using -Wno-attributes.  Observed with [trunk revision 142601] and [gcc-4_3-branch revision 135713].


