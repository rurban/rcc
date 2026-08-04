/* GCC Bug #78989 - Missing -Waddress warning due to -Wno-system-headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78989
 */
/* { dg-do compile } */
/* { dg-options "-Wall" } */

// The original reproducer (comment #0, and re-confirmed in comment #9) is a
// preprocessed C++ translation unit (gimplify.ii) taken from GCC's own
// gimplify.cpp, using the C++-only __null builtin as the NULL-pointer
// operand and a raw line marker that (re)enters what is treated as a system
// header at the point of that operand:
//
//   int
//   asan_poison_variables ()
//   {
//    return (asan_poison_variables &&
//   # 6 "gimplify.cpp" 3 4
//                                 __null
//                                     );
//   }
//
// NOTE: __null is a C++-only builtin (the underlying representation of NULL
// in libstdc++); it is not a valid expression for the plain C front end
// exercised by this reproducer, so it is replaced below with 0, a portable
// null-pointer constant, while keeping the rest of the reproducer (in
// particular the line marker placing the operand in a "system header")
// verbatim. This still triggers the reported -Waddress warning with a
// current compiler (as re-confirmed in comment #9 for GCC 12), i.e. the
// warning is no longer suppressed by the system-header line marker.

int
asan_poison_variables ()
{
 return (asan_poison_variables &&   /* { dg-warning "address of .asan_poison_variables. will always evaluate" } */
# 6 "gimplify.cpp" 3 4
                              0
                                  );
}
