// SPDX-License-Identifier: LGPL-2.1-or-later
// xmmintrin.h — Intel SSE (streaming SIMD) intrinsics for rcc.
//
// Built on rcc's __attribute__((__vector_size__)) support: the arithmetic,
// bitwise and comparison intrinsics lower to native packed SSE instructions
// (addps/mulps/andps/cmpps/...). The few intrinsics that are not expressible
// as C operators (movemask, shuffle, min/max, sqrt, ...) are implemented in
// terms of lane access, which the compiler still keeps in the vector domain.
#ifndef _XMMINTRIN_H_INCLUDED
#define _XMMINTRIN_H_INCLUDED

typedef float __m128 __attribute__((__vector_size__(16), __may_alias__));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef int __v4si __attribute__((__vector_size__(16)));

#define __rcc_inline static __inline__ __attribute__((__always_inline__, __unused__))

// --- Set / initialize ------------------------------------------------------
__rcc_inline __m128 _mm_set_ps(float __z, float __y, float __x, float __w) {
    return (__m128){__w, __x, __y, __z};
}
__rcc_inline __m128 _mm_setr_ps(float __w, float __x, float __y, float __z) {
    return (__m128){__w, __x, __y, __z};
}
__rcc_inline __m128 _mm_set1_ps(float __w) { return (__m128){__w, __w, __w, __w}; }
__rcc_inline __m128 _mm_set_ps1(float __w) { return _mm_set1_ps(__w); }
__rcc_inline __m128 _mm_setzero_ps(void) { return (__m128){0.0f, 0.0f, 0.0f, 0.0f}; }
__rcc_inline __m128 _mm_set_ss(float __w) { return (__m128){__w, 0.0f, 0.0f, 0.0f}; }

// --- Load / store ----------------------------------------------------------
__rcc_inline __m128 _mm_load_ps(const float *__p) { return *(const __m128 *)__p; }
__rcc_inline __m128 _mm_loadu_ps(const float *__p) { return *(const __m128 *)__p; }
__rcc_inline __m128 _mm_load_ps1(const float *__p) { return _mm_set1_ps(*__p); }
__rcc_inline __m128 _mm_load1_ps(const float *__p) { return _mm_set1_ps(*__p); }
__rcc_inline __m128 _mm_load_ss(const float *__p) { return _mm_set_ss(*__p); }
__rcc_inline __m128 _mm_loadr_ps(const float *__p) {
    return (__m128){__p[3], __p[2], __p[1], __p[0]};
}
__rcc_inline void _mm_store_ps(float *__p, __m128 __a) { *(__m128 *)__p = __a; }
__rcc_inline void _mm_storeu_ps(float *__p, __m128 __a) { *(__m128 *)__p = __a; }
__rcc_inline void _mm_store_ss(float *__p, __m128 __a) { *__p = __a[0]; }
__rcc_inline void _mm_store1_ps(float *__p, __m128 __a) {
    __m128 __v = _mm_set1_ps(__a[0]);
    *(__m128 *)__p = __v;
}
__rcc_inline void _mm_store_ps1(float *__p, __m128 __a) { _mm_store1_ps(__p, __a); }
__rcc_inline void _mm_storer_ps(float *__p, __m128 __a) {
    *(__m128 *)__p = (__m128){__a[3], __a[2], __a[1], __a[0]};
}

// --- Packed arithmetic (native SSE) ----------------------------------------
__rcc_inline __m128 _mm_add_ps(__m128 __a, __m128 __b) { return __a + __b; }
__rcc_inline __m128 _mm_sub_ps(__m128 __a, __m128 __b) { return __a - __b; }
__rcc_inline __m128 _mm_mul_ps(__m128 __a, __m128 __b) { return __a * __b; }
__rcc_inline __m128 _mm_div_ps(__m128 __a, __m128 __b) { return __a / __b; }

// --- Scalar arithmetic (lane 0 only) ---------------------------------------
__rcc_inline __m128 _mm_add_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] + __b[0];
    return __a;
}
__rcc_inline __m128 _mm_sub_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] - __b[0];
    return __a;
}
__rcc_inline __m128 _mm_mul_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] * __b[0];
    return __a;
}
__rcc_inline __m128 _mm_div_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] / __b[0];
    return __a;
}

// --- Bitwise (native SSE) --------------------------------------------------
__rcc_inline __m128 _mm_and_ps(__m128 __a, __m128 __b) { return __a & __b; }
__rcc_inline __m128 _mm_or_ps(__m128 __a, __m128 __b) { return __a | __b; }
__rcc_inline __m128 _mm_xor_ps(__m128 __a, __m128 __b) { return __a ^ __b; }
__rcc_inline __m128 _mm_andnot_ps(__m128 __a, __m128 __b) { return (~__a) & __b; }

// --- Comparisons (native SSE, produce per-lane all-ones/zero masks) ---------
__rcc_inline __m128 _mm_cmpeq_ps(__m128 __a, __m128 __b) { return __a == __b; }
__rcc_inline __m128 _mm_cmpneq_ps(__m128 __a, __m128 __b) { return __a != __b; }
__rcc_inline __m128 _mm_cmplt_ps(__m128 __a, __m128 __b) { return __a < __b; }
__rcc_inline __m128 _mm_cmple_ps(__m128 __a, __m128 __b) { return __a <= __b; }
__rcc_inline __m128 _mm_cmpgt_ps(__m128 __a, __m128 __b) { return __a > __b; }
__rcc_inline __m128 _mm_cmpge_ps(__m128 __a, __m128 __b) { return __a >= __b; }

// --- Min / max -------------------------------------------------------------
__rcc_inline __m128 _mm_min_ps(__m128 __a, __m128 __b) {
    __m128 __r;
    for (int __i = 0; __i < 4; __i++)
        __r[__i] = __a[__i] < __b[__i] ? __a[__i] : __b[__i];
    return __r;
}
__rcc_inline __m128 _mm_max_ps(__m128 __a, __m128 __b) {
    __m128 __r;
    for (int __i = 0; __i < 4; __i++)
        __r[__i] = __a[__i] > __b[__i] ? __a[__i] : __b[__i];
    return __r;
}
__rcc_inline __m128 _mm_min_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] < __b[0] ? __a[0] : __b[0];
    return __a;
}
__rcc_inline __m128 _mm_max_ss(__m128 __a, __m128 __b) {
    __a[0] = __a[0] > __b[0] ? __a[0] : __b[0];
    return __a;
}

// --- Reciprocal ------------------------------------------------------------
__rcc_inline __m128 _mm_rcp_ps(__m128 __a) {
    __m128 __r;
    for (int __i = 0; __i < 4; __i++)
        __r[__i] = 1.0f / __a[__i];
    return __r;
}
// TODO: _mm_sqrt_ps/_mm_sqrt_ss/_mm_rsqrt_ps need a native sqrtps builtin.
// A libm-based implementation is intentionally omitted: rcc does not
// dead-code-eliminate unused static inlines, so referencing sqrtf here would
// force every xmmintrin.h user to link -lm even when not using sqrt. The
// sqrtps/rsqrtps x86 encoders already exist (x86_sqrtps/x86_rsqrtps); wiring a
// __builtin_ia32_sqrtps that emits them is the proper fix.

// --- Shuffle / unpack ------------------------------------------------------
#define _MM_SHUFFLE(fp3, fp2, fp1, fp0) \
    (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))

__rcc_inline __m128 _mm_shuffle_ps(__m128 __a, __m128 __b, unsigned __imm) {
    return (__m128){__a[__imm & 3], __a[(__imm >> 2) & 3],
                    __b[(__imm >> 4) & 3], __b[(__imm >> 6) & 3]};
}
__rcc_inline __m128 _mm_unpackhi_ps(__m128 __a, __m128 __b) {
    return (__m128){__a[2], __b[2], __a[3], __b[3]};
}
__rcc_inline __m128 _mm_unpacklo_ps(__m128 __a, __m128 __b) {
    return (__m128){__a[0], __b[0], __a[1], __b[1]};
}
__rcc_inline __m128 _mm_movehl_ps(__m128 __a, __m128 __b) {
    return (__m128){__b[2], __b[3], __a[2], __a[3]};
}
__rcc_inline __m128 _mm_movelh_ps(__m128 __a, __m128 __b) {
    return (__m128){__a[0], __a[1], __b[0], __b[1]};
}

// --- Conversions / extraction ----------------------------------------------
__rcc_inline float _mm_cvtss_f32(__m128 __a) { return __a[0]; }

__rcc_inline int _mm_movemask_ps(__m128 __a) {
    union {
        __m128 __v;
        unsigned __u[4];
    } __m;
    __m.__v = __a;
    return (int)((__m.__u[0] >> 31) | ((__m.__u[1] >> 31) << 1) |
                 ((__m.__u[2] >> 31) << 2) | ((__m.__u[3] >> 31) << 3));
}

#undef __rcc_inline
#endif // _XMMINTRIN_H_INCLUDED
