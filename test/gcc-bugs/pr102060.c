/* GCC Bug #102060 - -Wprio-ctor-dtor underlines the wrong part of the source line due to missing location for attributes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102060
 */


// We get a warning when building for windows that should be fixed, but it highlights that the warning is slightly mistaken about where the problem is:
// gcc-git/libgcc/config/i386/cygming-crtend.c:59:1: warning: constructor priorities from 0 to 100 are reserved for the implementation [-Wprio-ctor-dtor]
//    59 | static void register_frame_ctor (void) __attribute__ ((constructor (0)));
//       | ^~~~~~
// The word "static" is underlined, but I presume that instead it should be "constructor(0)".


