// SPDX-License-Identifier: LGPL-2.1-or-later
// tmmintrin.h — Intel SSSE3 intrinsics for rcc.
//
// Like <emmintrin.h>, this is built on rcc's __attribute__((__vector_size__))
// support: everything that's expressible as plain lane-wise C arithmetic
// (abs, sign, horizontal add/sub, byte-align) is implemented that way.
// _mm_shuffle_epi8 is a genuine data-dependent byte-lane permute with no
// simple arithmetic equivalent, so it's the one intrinsic here backed by a
// real machine instruction (PSHUFB, via the __builtin_ia32_pshufb128
// compiler builtin, x86-only — see codegen.c's gen_vector_binary_builtin).
//
// The MMX-suffixed (`_pi8`/`_pi16`/`_pi32`) SSSE3 forms are intentionally
// omitted: rcc has no <mmintrin.h> / general __m64 arithmetic support (only
// the narrow float-pair-store __m64 use from <xmmintrin.h>'s
// _mm_storeh_pi/_mm_storel_pi), and no third-party project this project's
// test suite builds needs the MMX-suffixed forms.
#ifndef _TMMINTRIN_H_INCLUDED
#define _TMMINTRIN_H_INCLUDED

#include <emmintrin.h>

#ifndef __rcc_inline
#define __rcc_inline static __inline__ __attribute__((__always_inline__, __unused__))
#endif

// --- Byte-lane shuffle/permute (PSHUFB) -------------------------------------
// Per byte lane i: result[i] = (b[i] & 0x80) ? 0 : a[b[i] & 0x0f].
#ifndef ARCH_ARM64
__m128i __builtin_ia32_pshufb128(__m128i, __m128i);
__rcc_inline __m128i _mm_shuffle_epi8(__m128i __a, __m128i __b) {
    return __builtin_ia32_pshufb128(__a, __b);
}
#endif

// --- Absolute value ----------------------------------------------------------
__rcc_inline __m128i _mm_abs_epi8(__m128i __a) {
    __v16qs __x = (__v16qs)__a;
    for (int __i = 0; __i < 16; __i++)
        if (__x[__i] < 0) __x[__i] = (signed char)-__x[__i];
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_abs_epi16(__m128i __a) {
    __v8hi __x = (__v8hi)__a;
    for (int __i = 0; __i < 8; __i++)
        if (__x[__i] < 0) __x[__i] = (short)-__x[__i];
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_abs_epi32(__m128i __a) {
    __v4si_e __x = (__v4si_e)__a;
    for (int __i = 0; __i < 4; __i++)
        if (__x[__i] < 0) __x[__i] = -__x[__i];
    return (__m128i)__x;
}

// --- Conditional negate (sign) -----------------------------------------------
// Per lane: result = b[i] < 0 ? -a[i] : b[i] == 0 ? 0 : a[i].
__rcc_inline __m128i _mm_sign_epi8(__m128i __a, __m128i __b) {
    __v16qs __x = (__v16qs)__a, __s = (__v16qs)__b, __r;
    for (int __i = 0; __i < 16; __i++)
        __r[__i] = __s[__i] < 0 ? (signed char)-__x[__i] : __s[__i] == 0 ? 0
                                                                         : __x[__i];
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_sign_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __s = (__v8hi)__b, __r;
    for (int __i = 0; __i < 8; __i++)
        __r[__i] = __s[__i] < 0 ? (short)-__x[__i] : __s[__i] == 0 ? 0
                                                                   : __x[__i];
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_sign_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __s = (__v4si_e)__b, __r;
    for (int __i = 0; __i < 4; __i++)
        __r[__i] = __s[__i] < 0 ? -__x[__i] : __s[__i] == 0 ? 0
                                                            : __x[__i];
    return (__m128i)__r;
}

// --- Horizontal add/subtract --------------------------------------------------
__rcc_inline __m128i _mm_hadd_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b, __r;
    __r[0] = (short)(__x[0] + __x[1]);
    __r[1] = (short)(__x[2] + __x[3]);
    __r[2] = (short)(__x[4] + __x[5]);
    __r[3] = (short)(__x[6] + __x[7]);
    __r[4] = (short)(__y[0] + __y[1]);
    __r[5] = (short)(__y[2] + __y[3]);
    __r[6] = (short)(__y[4] + __y[5]);
    __r[7] = (short)(__y[6] + __y[7]);
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_hadd_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __y = (__v4si_e)__b, __r;
    __r[0] = __x[0] + __x[1];
    __r[1] = __x[2] + __x[3];
    __r[2] = __y[0] + __y[1];
    __r[3] = __y[2] + __y[3];
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_hsub_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b, __r;
    __r[0] = (short)(__x[0] - __x[1]);
    __r[1] = (short)(__x[2] - __x[3]);
    __r[2] = (short)(__x[4] - __x[5]);
    __r[3] = (short)(__x[6] - __x[7]);
    __r[4] = (short)(__y[0] - __y[1]);
    __r[5] = (short)(__y[2] - __y[3]);
    __r[6] = (short)(__y[4] - __y[5]);
    __r[7] = (short)(__y[6] - __y[7]);
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_hsub_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __y = (__v4si_e)__b, __r;
    __r[0] = __x[0] - __x[1];
    __r[1] = __x[2] - __x[3];
    __r[2] = __y[0] - __y[1];
    __r[3] = __y[2] - __y[3];
    return (__m128i)__r;
}
// Saturating horizontal add/subtract (16-bit lanes)
__rcc_inline short __rcc_ssat16(int __v) { return __v > 32767 ? 32767 : __v < -32768 ? -32768
                                                                                     : (short)__v; }
__rcc_inline __m128i _mm_hadds_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b, __r;
    __r[0] = __rcc_ssat16(__x[0] + __x[1]);
    __r[1] = __rcc_ssat16(__x[2] + __x[3]);
    __r[2] = __rcc_ssat16(__x[4] + __x[5]);
    __r[3] = __rcc_ssat16(__x[6] + __x[7]);
    __r[4] = __rcc_ssat16(__y[0] + __y[1]);
    __r[5] = __rcc_ssat16(__y[2] + __y[3]);
    __r[6] = __rcc_ssat16(__y[4] + __y[5]);
    __r[7] = __rcc_ssat16(__y[6] + __y[7]);
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_hsubs_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b, __r;
    __r[0] = __rcc_ssat16(__x[0] - __x[1]);
    __r[1] = __rcc_ssat16(__x[2] - __x[3]);
    __r[2] = __rcc_ssat16(__x[4] - __x[5]);
    __r[3] = __rcc_ssat16(__x[6] - __x[7]);
    __r[4] = __rcc_ssat16(__y[0] - __y[1]);
    __r[5] = __rcc_ssat16(__y[2] - __y[3]);
    __r[6] = __rcc_ssat16(__y[4] - __y[5]);
    __r[7] = __rcc_ssat16(__y[6] - __y[7]);
    return (__m128i)__r;
}

// --- Byte-align (PALIGNR emulation) -------------------------------------------
// Concatenates b:a (b high, a low) as a 32-byte buffer and returns the
// 16 bytes starting at byte offset __imm; an offset >= 32 yields all-zero.
__rcc_inline __m128i _mm_alignr_epi8(__m128i __a, __m128i __b, int __imm) {
    unsigned char __buf[32];
    if (__imm >= 32) return _mm_setzero_si128();
    __v16qu __va = (__v16qu)__a, __vb = (__v16qu)__b;
    for (int __i = 0; __i < 16; __i++) {
        __buf[__i] = __vb[__i];
        __buf[__i + 16] = __va[__i];
    }
    __v16qu __r;
    for (int __i = 0; __i < 16; __i++)
        __r[__i] = (__imm + __i < 32) ? __buf[__imm + __i] : 0;
    return (__m128i)__r;
}

// --- Multiply/add (signed*unsigned -> saturated 16-bit) -----------------------
// __a: unsigned bytes, __b: signed bytes; pairwise products summed per
// 16-bit lane, saturated to the signed 16-bit range.
__rcc_inline __m128i _mm_maddubs_epi16(__m128i __a, __m128i __b) {
    __v16qu __ua = (__v16qu)__a;
    __v16qs __sb = (__v16qs)__b;
    __v8hi __r;
    for (int __i = 0; __i < 8; __i++) {
        int __sum = (int)__ua[__i * 2] * __sb[__i * 2] + (int)__ua[__i * 2 + 1] * __sb[__i * 2 + 1];
        __r[__i] = __rcc_ssat16(__sum);
    }
    return (__m128i)__r;
}

// --- Rounded, scaled multiply-high (Q15 fixed-point) --------------------------
__rcc_inline __m128i _mm_mulhrs_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b, __r;
    for (int __i = 0; __i < 8; __i++) {
        int __p = (int)__x[__i] * (int)__y[__i];
        __r[__i] = (short)(((__p >> 14) + 1) >> 1);
    }
    return (__m128i)__r;
}

#endif // _TMMINTRIN_H_INCLUDED
