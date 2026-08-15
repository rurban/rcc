// SPDX-License-Identifier: LGPL-2.1-or-later
// immintrin.h — Intel AVX/AVX2 and later SIMD intrinsics umbrella for rcc.
//
// rcc implements the __builtin_ia32_* builtins that GCC's AVX/AVX2 headers
// use, but it still cannot parse some SSE4.x inline definitions (the
// _mm_extract_epi{8,16,32,64} helpers in <smmintrin.h> use a const int
// parameter where the underlying builtin requires an immediate constant).
// We therefore provide a subset umbrella that includes the headers that
// work and temporarily hides __OPTIMIZE__ around <smmintrin.h> so that the
// problematic inline definitions are skipped and only the macro forms are
// left.  AVX-512 and the more exotic extensions are omitted for now; include
// their specific headers directly if needed.
#ifndef _IMMINTRIN_H_INCLUDED
#define _IMMINTRIN_H_INCLUDED
// Clang's headers use a different guard name than GCC's.
#ifndef __IMMINTRIN_H
#define __IMMINTRIN_H
#endif

#include <x86gprintrin.h>
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>

// See comment above.  Several headers below define inline helpers that pass
// a runtime parameter to a builtin that needs an immediate constant
// (_mm_extract_epi*, _mm_clmulepi64_si128, _mm256_extract_epi*,
// _mm256_permute*, _mm256_cvtps_ph, ...).  Each such block in these headers
// has a macro `#else` alternative that still works when the caller passes a
// constant, so hide __OPTIMIZE__ across all of them and expose only the
// macro forms.
#ifdef __OPTIMIZE__
#define __rcc_saved_optimize __OPTIMIZE__
#undef __OPTIMIZE__
#endif
#include <smmintrin.h>
#include <wmmintrin.h>
#include <avxintrin.h>
#include <avxvnniintrin.h>
#include <avxifmaintrin.h>
#include <avxvnniint8intrin.h>
#include <avxvnniint16intrin.h>
#include <avx2intrin.h>
#include <f16cintrin.h>
#ifdef __rcc_saved_optimize
#define __OPTIMIZE__ __rcc_saved_optimize
#undef __rcc_saved_optimize
#endif
#include <fmaintrin.h>
#include <rtmintrin.h>
#include <bmiintrin.h>
#include <bmi2intrin.h>

#endif /* _IMMINTRIN_H_INCLUDED */
