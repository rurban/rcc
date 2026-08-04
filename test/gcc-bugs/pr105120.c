/* GCC Bug #105120 - __OPTIMIZE__ macro incorrectly defined when using pragma(optimize) with push_options/pop_options
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105120
 */


inline void noop() {}
#pragma message("__FAST_MATH__ defined")
#endif
// so pop_options fails to pop the macro definitions.


