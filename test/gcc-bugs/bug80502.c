/* GCC Bug #80502 - Provide macro to indicate OpenMP SIMD support
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80502
 */
/* { dg-do compile } */


#pragma omp simd
  #pragma simd
  #pragma GCC ivdep
  #pragma clang loop vectorize(enable)
  for (...) { ... }

// I'd much rather just have a few macros which will expand to the right pragma based on preprocessor macros. Right now I'm stuck using the much less expressive ivdep syntax for GCC unless *full* OpenMP support is enabled (or someone defines a macro manually to indicate OpenMP SIMD support).


