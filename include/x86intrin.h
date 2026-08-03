// SPDX-License-Identifier: LGPL-2.1-or-later
// x86intrin.h — umbrella x86 SIMD intrinsics header for rcc.
//
// gcc's own <x86intrin.h> pulls in the entire immintrin.h SSE/AVX tree, whose
// inline functions use clang/gcc target builtins (__builtin_shufflevector,
// __builtin_ia32_movsd, ...) that rcc does not implement.  rcc ships its own
// SSE (xmmintrin.h) and SSE2 (emmintrin.h) implementations built on native
// __attribute__((__vector_size__)) support, so this umbrella simply pulls in
// those.  It is what <winnt.h> (via <windows.h>) includes on Windows.
#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED

#include <xmmintrin.h>
#include <emmintrin.h>

#endif /* _X86INTRIN_H_INCLUDED */
