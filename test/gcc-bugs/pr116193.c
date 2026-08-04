/* GCC Bug #116193 - enhancement: type attribute that causes overflow for unsigned integer types to trap
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116193
 */


typedef unsigned int __attribute__ ((__overflow__)) positive_int;


