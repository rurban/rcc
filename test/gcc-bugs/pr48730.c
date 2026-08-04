/* GCC Bug #48730 - static function scope not honored by -fms-extensions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48730
 */
/* { dg-do compile } */


extern int clip();
static int clip()
{}
// which gcc fails to compile:
// clip.c:2:12: error: static declaration of 'clip' follows non-static declaration
// clip.c:1:12: note: previous declaration of 'clip' was here
// The original code which encountered this error wasn't this simple.  The 'extern int function();' was buried in some deeply nested layers of system include files, and the locally scoped function had a name which was identical to the one defined in the include file.
// The code compiles cleanly with Microsoft's compilers but fails with the above error when compiled with gcc.
// It would seem appropriate that this behavior like the microsoft compilers should be part of the functionality of '-fms-extensions'.


