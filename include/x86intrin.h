/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Stub: prevents gcc's x86intrin.h from pulling in immintrin.h and the
 * entire SSE/AVX intrinsic header tree (emmintrin.h, avx2intrin.h, ...).
 * Those headers contain inline functions that use gcc/clang builtins
 * (__builtin_ia32_movsd, __builtin_shufflevector, ...) which rcc does
 * not support.
 *
 * This is sufficient for any code that includes <windows.h> but never
 * calls x86 SIMD intrinsics — which is the common case on Windows
 * where the CRT's winnt.h includes x86intrin.h unconditionally. */

#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED
#endif /* _X86INTRIN_H_INCLUDED */
