/* GCC Bug #54408 - sqrt for vector types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54408
 */
/* { dg-do compile } */


/* The original report (Marc Glisse, 2012) is a pure feature-request
 * discussion with no attached testcase: "it would be nice to add sqrt
 * to the gcc vector extensions... Component 'other' because fixing
 * this might involve front, middle and back-ends." Neither GCC nor
 * Clang has a vector-aware sqrt: math.h's sqrt(double) rejects a
 * vector argument outright. This is a faithful minimal C illustration
 * of that gap (the previously scraped file used C++ template syntax
 * that was never part of the actual report and isn't valid C at all). */
#include <math.h>
typedef double v2d __attribute__ ((vector_size (16)));
v2d f(v2d x) { return x + sqrt(x); } /* { dg-error "incompatible type" } */
