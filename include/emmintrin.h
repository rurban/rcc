// SPDX-License-Identifier: LGPL-2.1-or-later
// emmintrin.h — Intel SSE2 (double / packed-integer SIMD) intrinsics for rcc.
//
// Like <xmmintrin.h>, this is built on rcc's __attribute__((__vector_size__))
// support: arithmetic, bitwise and comparison intrinsics lower to native
// packed SSE2 instructions (addpd/paddd/pand/cmppd/...).  The intrinsics that
// are not expressible as C operators (movemask, shuffle, sqrt, ...) are
// implemented via lane access or the __builtin_ia32_sqrt{pd,sd} builtins,
// which the compiler keeps in the vector domain.
//
// <winnt.h> (via <windows.h>) includes this header directly to make the
// SSE2 types and intrinsics available on Windows; the memory-fence / pause
// intrinsics it uses (_mm_pause, _mm_mfence, ...) come from mingw's own
// <intrin.h>, so they are intentionally NOT redefined here.
#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED

#include <xmmintrin.h>

// SSE2 vector types.  __m128d / __m128i alias any type (may_alias); the __vNN
// typedefs are the typed lane views used to reinterpret the raw registers.
typedef double __m128d __attribute__((__vector_size__(16), __may_alias__));
typedef long long __m128i __attribute__((__vector_size__(16), __may_alias__));

typedef double __v2df __attribute__((__vector_size__(16)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
typedef int __v4si_e __attribute__((__vector_size__(16)));
typedef unsigned int __v4su __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));
typedef signed char __v16qs __attribute__((__vector_size__(16)));
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));

#define __rcc_inline static __inline__ __attribute__((__always_inline__, __unused__))

// --- Reinterpret casts (no code) -------------------------------------------
__rcc_inline __m128 _mm_castpd_ps(__m128d __a) { return (__m128)__a; }
__rcc_inline __m128i _mm_castpd_si128(__m128d __a) { return (__m128i)__a; }
__rcc_inline __m128d _mm_castps_pd(__m128 __a) { return (__m128d)__a; }
__rcc_inline __m128i _mm_castps_si128(__m128 __a) { return (__m128i)__a; }
__rcc_inline __m128 _mm_castsi128_ps(__m128i __a) { return (__m128)__a; }
__rcc_inline __m128d _mm_castsi128_pd(__m128i __a) { return (__m128d)__a; }

// ===========================================================================
// Double precision (__m128d)
// ===========================================================================

// --- Set / initialize ------------------------------------------------------
__rcc_inline __m128d _mm_set_pd(double __y, double __x) { return (__m128d){__x, __y}; }
__rcc_inline __m128d _mm_setr_pd(double __x, double __y) { return (__m128d){__x, __y}; }
__rcc_inline __m128d _mm_set1_pd(double __x) { return (__m128d){__x, __x}; }
__rcc_inline __m128d _mm_set_pd1(double __x) { return _mm_set1_pd(__x); }
__rcc_inline __m128d _mm_setzero_pd(void) { return (__m128d){0.0, 0.0}; }
__rcc_inline __m128d _mm_set_sd(double __x) { return (__m128d){__x, 0.0}; }

// --- Load / store ----------------------------------------------------------
__rcc_inline __m128d _mm_load_pd(const double *__p) { return *(const __m128d *)__p; }
__rcc_inline __m128d _mm_loadu_pd(const double *__p) { return *(const __m128d *)__p; }
__rcc_inline __m128d _mm_load1_pd(const double *__p) { return _mm_set1_pd(*__p); }
__rcc_inline __m128d _mm_load_pd1(const double *__p) { return _mm_set1_pd(*__p); }
__rcc_inline __m128d _mm_load_sd(const double *__p) { return (__m128d){*__p, 0.0}; }
__rcc_inline __m128d _mm_loadr_pd(const double *__p) { return (__m128d){__p[1], __p[0]}; }
__rcc_inline __m128d _mm_loadh_pd(__m128d __a, const double *__p) {
    __a[1] = *__p;
    return __a;
}
__rcc_inline __m128d _mm_loadl_pd(__m128d __a, const double *__p) {
    __a[0] = *__p;
    return __a;
}
__rcc_inline void _mm_store_pd(double *__p, __m128d __a) { *(__m128d *)__p = __a; }
__rcc_inline void _mm_storeu_pd(double *__p, __m128d __a) { *(__m128d *)__p = __a; }
__rcc_inline void _mm_store_sd(double *__p, __m128d __a) { *__p = __a[0]; }
__rcc_inline void _mm_store1_pd(double *__p, __m128d __a) { *(__m128d *)__p = _mm_set1_pd(__a[0]); }
__rcc_inline void _mm_store_pd1(double *__p, __m128d __a) { _mm_store1_pd(__p, __a); }
__rcc_inline void _mm_storer_pd(double *__p, __m128d __a) {
    *(__m128d *)__p = (__m128d){__a[1], __a[0]};
}
__rcc_inline void _mm_storeh_pd(double *__p, __m128d __a) { *__p = __a[1]; }
__rcc_inline void _mm_storel_pd(double *__p, __m128d __a) { *__p = __a[0]; }

// --- Packed / scalar arithmetic --------------------------------------------
__rcc_inline __m128d _mm_add_pd(__m128d __a, __m128d __b) { return __a + __b; }
__rcc_inline __m128d _mm_sub_pd(__m128d __a, __m128d __b) { return __a - __b; }
__rcc_inline __m128d _mm_mul_pd(__m128d __a, __m128d __b) { return __a * __b; }
__rcc_inline __m128d _mm_div_pd(__m128d __a, __m128d __b) { return __a / __b; }
__rcc_inline __m128d _mm_add_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] + __b[0];
    return __a;
}
__rcc_inline __m128d _mm_sub_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] - __b[0];
    return __a;
}
__rcc_inline __m128d _mm_mul_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] * __b[0];
    return __a;
}
__rcc_inline __m128d _mm_div_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] / __b[0];
    return __a;
}

// --- Square root (native sqrtpd/sqrtsd builtin, no -lm) ---------------------
__m128d __builtin_ia32_sqrtpd(__m128d);
__m128d __builtin_ia32_sqrtsd(__m128d);
__rcc_inline __m128d _mm_sqrt_pd(__m128d __a) { return __builtin_ia32_sqrtpd(__a); }
__rcc_inline __m128d _mm_sqrt_sd(__m128d __a, __m128d __b) {
    __m128d __r = __builtin_ia32_sqrtsd(__b); // {sqrt(b0), b1}
    __r[1] = __a[1];
    return __r;
}

// --- Min / max -------------------------------------------------------------
__rcc_inline __m128d _mm_min_pd(__m128d __a, __m128d __b) {
    return (__m128d){__a[0] < __b[0] ? __a[0] : __b[0], __a[1] < __b[1] ? __a[1] : __b[1]};
}
__rcc_inline __m128d _mm_max_pd(__m128d __a, __m128d __b) {
    return (__m128d){__a[0] > __b[0] ? __a[0] : __b[0], __a[1] > __b[1] ? __a[1] : __b[1]};
}
__rcc_inline __m128d _mm_min_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] < __b[0] ? __a[0] : __b[0];
    return __a;
}
__rcc_inline __m128d _mm_max_sd(__m128d __a, __m128d __b) {
    __a[0] = __a[0] > __b[0] ? __a[0] : __b[0];
    return __a;
}

// --- Bitwise ---------------------------------------------------------------
__rcc_inline __m128d _mm_and_pd(__m128d __a, __m128d __b) { return __a & __b; }
__rcc_inline __m128d _mm_or_pd(__m128d __a, __m128d __b) { return __a | __b; }
__rcc_inline __m128d _mm_xor_pd(__m128d __a, __m128d __b) { return __a ^ __b; }
__rcc_inline __m128d _mm_andnot_pd(__m128d __a, __m128d __b) { return (~__a) & __b; }

// --- Comparisons (per-lane all-ones/zero masks) ----------------------------
__rcc_inline __m128d _mm_cmpeq_pd(__m128d __a, __m128d __b) { return (__m128d)(__a == __b); }
__rcc_inline __m128d _mm_cmpneq_pd(__m128d __a, __m128d __b) { return (__m128d)(__a != __b); }
__rcc_inline __m128d _mm_cmplt_pd(__m128d __a, __m128d __b) { return (__m128d)(__a < __b); }
__rcc_inline __m128d _mm_cmple_pd(__m128d __a, __m128d __b) { return (__m128d)(__a <= __b); }
__rcc_inline __m128d _mm_cmpgt_pd(__m128d __a, __m128d __b) { return (__m128d)(__a > __b); }
__rcc_inline __m128d _mm_cmpge_pd(__m128d __a, __m128d __b) { return (__m128d)(__a >= __b); }

// --- Shuffle / unpack ------------------------------------------------------
__rcc_inline __m128d _mm_unpackhi_pd(__m128d __a, __m128d __b) { return (__m128d){__a[1], __b[1]}; }
__rcc_inline __m128d _mm_unpacklo_pd(__m128d __a, __m128d __b) { return (__m128d){__a[0], __b[0]}; }
__rcc_inline __m128d _mm_move_sd(__m128d __a, __m128d __b) { return (__m128d){__b[0], __a[1]}; }
__rcc_inline __m128d _mm_shuffle_pd(__m128d __a, __m128d __b, unsigned __imm) {
    return (__m128d){__a[__imm & 1], __b[(__imm >> 1) & 1]};
}

// --- Extraction / movemask -------------------------------------------------
__rcc_inline double _mm_cvtsd_f64(__m128d __a) { return __a[0]; }
__rcc_inline int _mm_movemask_pd(__m128d __a) {
    union {
        __m128d __v;
        unsigned long long __u[2];
    } __m;
    __m.__v = __a;
    return (int)((__m.__u[0] >> 63) | ((__m.__u[1] >> 63) << 1));
}

// ===========================================================================
// Packed integer (__m128i)
// ===========================================================================

// --- Set / initialize ------------------------------------------------------
__rcc_inline __m128i _mm_set_epi64x(long long __q1, long long __q0) {
    return (__m128i){__q0, __q1};
}
__rcc_inline __m128i _mm_set_epi32(int __i3, int __i2, int __i1, int __i0) {
    return (__m128i)(__v4si_e){__i0, __i1, __i2, __i3};
}
__rcc_inline __m128i _mm_set_epi16(short __w7, short __w6, short __w5, short __w4, short __w3,
                                   short __w2, short __w1, short __w0) {
    return (__m128i)(__v8hi){__w0, __w1, __w2, __w3, __w4, __w5, __w6, __w7};
}
__rcc_inline __m128i _mm_set_epi8(char __b15, char __b14, char __b13, char __b12, char __b11,
                                  char __b10, char __b9, char __b8, char __b7, char __b6, char __b5,
                                  char __b4, char __b3, char __b2, char __b1, char __b0) {
    return (__m128i)(__v16qi){__b0, __b1, __b2, __b3, __b4, __b5, __b6, __b7,
                              __b8, __b9, __b10, __b11, __b12, __b13, __b14, __b15};
}
__rcc_inline __m128i _mm_setr_epi32(int __i0, int __i1, int __i2, int __i3) {
    return (__m128i)(__v4si_e){__i0, __i1, __i2, __i3};
}
__rcc_inline __m128i _mm_set1_epi64x(long long __q) { return (__m128i){__q, __q}; }
__rcc_inline __m128i _mm_set1_epi32(int __i) { return (__m128i)(__v4si_e){__i, __i, __i, __i}; }
__rcc_inline __m128i _mm_set1_epi16(short __w) {
    return (__m128i)(__v8hi){__w, __w, __w, __w, __w, __w, __w, __w};
}
__rcc_inline __m128i _mm_set1_epi8(char __b) {
    return (__m128i)(__v16qi){__b, __b, __b, __b, __b, __b, __b, __b,
                              __b, __b, __b, __b, __b, __b, __b, __b};
}
__rcc_inline __m128i _mm_setzero_si128(void) { return (__m128i){0, 0}; }

// --- Load / store ----------------------------------------------------------
__rcc_inline __m128i _mm_load_si128(const __m128i *__p) { return *__p; }
__rcc_inline __m128i _mm_loadu_si128(const __m128i *__p) { return *__p; }
__rcc_inline __m128i _mm_loadl_epi64(const __m128i *__p) {
    return (__m128i){((const long long *)__p)[0], 0};
}
__rcc_inline void _mm_store_si128(__m128i *__p, __m128i __a) { *__p = __a; }
__rcc_inline void _mm_storeu_si128(__m128i *__p, __m128i __a) { *__p = __a; }
__rcc_inline void _mm_storel_epi64(__m128i *__p, __m128i __a) { ((long long *)__p)[0] = __a[0]; }

// --- Integer arithmetic ----------------------------------------------------
__rcc_inline __m128i _mm_add_epi8(__m128i __a, __m128i __b) {
    return (__m128i)((__v16qi)__a + (__v16qi)__b);
}
__rcc_inline __m128i _mm_add_epi16(__m128i __a, __m128i __b) {
    return (__m128i)((__v8hi)__a + (__v8hi)__b);
}
__rcc_inline __m128i _mm_add_epi32(__m128i __a, __m128i __b) {
    return (__m128i)((__v4si_e)__a + (__v4si_e)__b);
}
__rcc_inline __m128i _mm_add_epi64(__m128i __a, __m128i __b) { return __a + __b; }
__rcc_inline __m128i _mm_sub_epi8(__m128i __a, __m128i __b) {
    return (__m128i)((__v16qi)__a - (__v16qi)__b);
}
__rcc_inline __m128i _mm_sub_epi16(__m128i __a, __m128i __b) {
    return (__m128i)((__v8hi)__a - (__v8hi)__b);
}
__rcc_inline __m128i _mm_sub_epi32(__m128i __a, __m128i __b) {
    return (__m128i)((__v4si_e)__a - (__v4si_e)__b);
}
__rcc_inline __m128i _mm_sub_epi64(__m128i __a, __m128i __b) { return __a - __b; }
__rcc_inline __m128i _mm_mullo_epi16(__m128i __a, __m128i __b) {
    return (__m128i)((__v8hi)__a * (__v8hi)__b);
}
// Multiply the low unsigned 32-bit of each 64-bit lane, yielding
// 64-bit unsigned products.
__rcc_inline __m128i _mm_mul_epu32(__m128i __a, __m128i __b) {
    __v4su __va = (__v4su)__a, __vb = (__v4su)__b;
    return (__m128i)(__v2du){(unsigned long long)__va[0] * __vb[0],
                             (unsigned long long)__va[2] * __vb[2]};
}
// Shuffle 32-bit lanes: imm is a 4x2-bit selector.
__rcc_inline __m128i _mm_shuffle_epi32(__m128i __a, unsigned __imm) {
    __v4si_e __v = (__v4si_e)__a;
    return (__m128i)(__v4si_e){__v[__imm & 3], __v[(__imm >> 2) & 3],
                               __v[(__imm >> 4) & 3], __v[(__imm >> 6) & 3]};
}
// Shuffle the low/high four 16-bit lanes independently: imm is a
// 4x2-bit selector for the shuffled half; the other half passes
// through unchanged.
__rcc_inline __m128i _mm_shufflelo_epi16(__m128i __a, unsigned __imm) {
    __v8hi __v = (__v8hi)__a;
    return (__m128i)(__v8hi){__v[__imm & 3], __v[(__imm >> 2) & 3], __v[(__imm >> 4) & 3],
                             __v[(__imm >> 6) & 3], __v[4], __v[5], __v[6], __v[7]};
}
__rcc_inline __m128i _mm_shufflehi_epi16(__m128i __a, unsigned __imm) {
    __v8hi __v = (__v8hi)__a;
    return (__m128i)(__v8hi){__v[0], __v[1], __v[2], __v[3],
                             __v[4 + (__imm & 3)], __v[4 + ((__imm >> 2) & 3)],
                             __v[4 + ((__imm >> 4) & 3)], __v[4 + ((__imm >> 6) & 3)]};
}

// --- Integer bitwise -------------------------------------------------------
__rcc_inline __m128i _mm_and_si128(__m128i __a, __m128i __b) { return __a & __b; }
__rcc_inline __m128i _mm_or_si128(__m128i __a, __m128i __b) { return __a | __b; }
__rcc_inline __m128i _mm_xor_si128(__m128i __a, __m128i __b) { return __a ^ __b; }
__rcc_inline __m128i _mm_andnot_si128(__m128i __a, __m128i __b) { return (~__a) & __b; }

// --- Integer comparisons ---------------------------------------------------
__rcc_inline __m128i _mm_cmpeq_epi8(__m128i __a, __m128i __b) {
    return (__m128i)((__v16qi)__a == (__v16qi)__b);
}
__rcc_inline __m128i _mm_cmpeq_epi16(__m128i __a, __m128i __b) {
    return (__m128i)((__v8hi)__a == (__v8hi)__b);
}
__rcc_inline __m128i _mm_cmpeq_epi32(__m128i __a, __m128i __b) {
    return (__m128i)((__v4si_e)__a == (__v4si_e)__b);
}
__rcc_inline __m128i _mm_cmpgt_epi8(__m128i __a, __m128i __b) {
    return (__m128i)((__v16qs)__a > (__v16qs)__b);
}
__rcc_inline __m128i _mm_cmpgt_epi16(__m128i __a, __m128i __b) {
    return (__m128i)((__v8hi)__a > (__v8hi)__b);
}
__rcc_inline __m128i _mm_cmpgt_epi32(__m128i __a, __m128i __b) {
    return (__m128i)((__v4si_e)__a > (__v4si_e)__b);
}
__rcc_inline __m128i _mm_cmplt_epi8(__m128i __a, __m128i __b) { return _mm_cmpgt_epi8(__b, __a); }
__rcc_inline __m128i _mm_cmplt_epi16(__m128i __a, __m128i __b) { return _mm_cmpgt_epi16(__b, __a); }
__rcc_inline __m128i _mm_cmplt_epi32(__m128i __a, __m128i __b) { return _mm_cmpgt_epi32(__b, __a); }

// --- Integer shifts by scalar count ----------------------------------------
// SSE semantics: a shift count outside the element width yields all-zero lanes
// (logical) or a full sign fill (arithmetic).
__rcc_inline __m128i _mm_slli_epi16(__m128i __a, int __c) {
    __v8hu __x = (__v8hu)__a;
    if (__c < 0 || __c > 15) return _mm_setzero_si128();
    for (int __i = 0; __i < 8; __i++) __x[__i] = (unsigned short)(__x[__i] << __c);
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_slli_epi32(__m128i __a, int __c) {
    __v4su __x = (__v4su)__a;
    if (__c < 0 || __c > 31) return _mm_setzero_si128();
    for (int __i = 0; __i < 4; __i++) __x[__i] = __x[__i] << __c;
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_slli_epi64(__m128i __a, int __c) {
    __v2du __x = (__v2du)__a;
    if (__c < 0 || __c > 63) return _mm_setzero_si128();
    for (int __i = 0; __i < 2; __i++) __x[__i] = __x[__i] << __c;
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_srli_epi16(__m128i __a, int __c) {
    __v8hu __x = (__v8hu)__a;
    if (__c < 0 || __c > 15) return _mm_setzero_si128();
    for (int __i = 0; __i < 8; __i++) __x[__i] = (unsigned short)(__x[__i] >> __c);
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_srli_epi32(__m128i __a, int __c) {
    __v4su __x = (__v4su)__a;
    if (__c < 0 || __c > 31) return _mm_setzero_si128();
    for (int __i = 0; __i < 4; __i++) __x[__i] = __x[__i] >> __c;
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_srli_epi64(__m128i __a, int __c) {
    __v2du __x = (__v2du)__a;
    if (__c < 0 || __c > 63) return _mm_setzero_si128();
    for (int __i = 0; __i < 2; __i++) __x[__i] = __x[__i] >> __c;
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_srai_epi16(__m128i __a, int __c) {
    __v8hi __x = (__v8hi)__a;
    int __s = (__c < 0 || __c > 15) ? 15 : __c;
    for (int __i = 0; __i < 8; __i++) __x[__i] = (short)(__x[__i] >> __s);
    return (__m128i)__x;
}
__rcc_inline __m128i _mm_srai_epi32(__m128i __a, int __c) {
    __v4si_e __x = (__v4si_e)__a;
    int __s = (__c < 0 || __c > 31) ? 31 : __c;
    for (int __i = 0; __i < 4; __i++) __x[__i] = __x[__i] >> __s;
    return (__m128i)__x;
}

// --- Extract / insert / movemask -------------------------------------------
__rcc_inline int _mm_extract_epi16(__m128i __a, int __imm) {
    return (unsigned short)((__v8hi)__a)[__imm & 7];
}
__rcc_inline __m128i _mm_insert_epi16(__m128i __a, int __v, int __imm) {
    __v8hi __x = (__v8hi)__a;
    __x[__imm & 7] = (short)__v;
    return (__m128i)__x;
}
__rcc_inline int _mm_movemask_epi8(__m128i __a) {
    __v16qu __x = (__v16qu)__a;
    int __r = 0;
    for (int __i = 0; __i < 16; __i++) __r |= (int)(__x[__i] >> 7) << __i;
    return __r;
}

// --- Interleave (unpack) low/high lanes -------------------------------------
__rcc_inline __m128i _mm_unpacklo_epi8(__m128i __a, __m128i __b) {
    __v16qi __x = (__v16qi)__a, __y = (__v16qi)__b, __r;
    for (int __i = 0; __i < 8; __i++) {
        __r[__i * 2] = __x[__i];
        __r[__i * 2 + 1] = __y[__i];
    }
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_unpackhi_epi8(__m128i __a, __m128i __b) {
    __v16qi __x = (__v16qi)__a, __y = (__v16qi)__b, __r;
    for (int __i = 0; __i < 8; __i++) {
        __r[__i * 2] = __x[__i + 8];
        __r[__i * 2 + 1] = __y[__i + 8];
    }
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_unpacklo_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b;
    return (__m128i)(__v8hi){__x[0], __y[0], __x[1], __y[1], __x[2], __y[2], __x[3], __y[3]};
}
__rcc_inline __m128i _mm_unpackhi_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b;
    return (__m128i)(__v8hi){__x[4], __y[4], __x[5], __y[5], __x[6], __y[6], __x[7], __y[7]};
}
__rcc_inline __m128i _mm_unpacklo_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __y = (__v4si_e)__b;
    return (__m128i)(__v4si_e){__x[0], __y[0], __x[1], __y[1]};
}
__rcc_inline __m128i _mm_unpackhi_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __y = (__v4si_e)__b;
    return (__m128i)(__v4si_e){__x[2], __y[2], __x[3], __y[3]};
}
__rcc_inline __m128i _mm_unpacklo_epi64(__m128i __a, __m128i __b) {
    return (__m128i){__a[0], __b[0]};
}
__rcc_inline __m128i _mm_unpackhi_epi64(__m128i __a, __m128i __b) {
    return (__m128i){__a[1], __b[1]};
}

// --- Saturating pack (two vectors' worth of wider lanes -> one narrower) ---
__rcc_inline __m128i _mm_packs_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b;
    __v16qs __r;
    for (int __i = 0; __i < 8; __i++) {
        int __v = __x[__i];
        __r[__i] = (signed char)(__v > 127 ? 127 : __v < -128 ? -128
                                                              : __v);
    }
    for (int __i = 0; __i < 8; __i++) {
        int __v = __y[__i];
        __r[__i + 8] = (signed char)(__v > 127 ? 127 : __v < -128 ? -128
                                                                  : __v);
    }
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_packus_epi16(__m128i __a, __m128i __b) {
    __v8hi __x = (__v8hi)__a, __y = (__v8hi)__b;
    __v16qu __r;
    for (int __i = 0; __i < 8; __i++) {
        int __v = __x[__i];
        __r[__i] = (unsigned char)(__v > 255 ? 255 : __v < 0 ? 0
                                                             : __v);
    }
    for (int __i = 0; __i < 8; __i++) {
        int __v = __y[__i];
        __r[__i + 8] = (unsigned char)(__v > 255 ? 255 : __v < 0 ? 0
                                                                 : __v);
    }
    return (__m128i)__r;
}
__rcc_inline __m128i _mm_packs_epi32(__m128i __a, __m128i __b) {
    __v4si_e __x = (__v4si_e)__a, __y = (__v4si_e)__b;
    __v8hi __r;
    for (int __i = 0; __i < 4; __i++) {
        int __v = __x[__i];
        __r[__i] = (short)(__v > 32767 ? 32767 : __v < -32768 ? -32768
                                                              : __v);
    }
    for (int __i = 0; __i < 4; __i++) {
        int __v = __y[__i];
        __r[__i + 4] = (short)(__v > 32767 ? 32767 : __v < -32768 ? -32768
                                                                  : __v);
    }
    return (__m128i)__r;
}

// --- Masked store: store only the lanes whose mask byte's high bit is set --
__rcc_inline void _mm_maskmoveu_si128(__m128i __a, __m128i __mask, char *__p) {
    __v16qi __x = (__v16qi)__a;
    __v16qu __m = (__v16qu)__mask;
    for (int __i = 0; __i < 16; __i++)
        if (__m[__i] & 0x80) __p[__i] = __x[__i];
}

// --- Integer <-> scalar conversions ----------------------------------------
__rcc_inline int _mm_cvtsi128_si32(__m128i __a) { return ((__v4si_e)__a)[0]; }
__rcc_inline long long _mm_cvtsi128_si64(__m128i __a) { return __a[0]; }
__rcc_inline __m128i _mm_cvtsi32_si128(int __a) { return (__m128i)(__v4si_e){__a, 0, 0, 0}; }
__rcc_inline __m128i _mm_cvtsi64_si128(long long __a) { return (__m128i){__a, 0}; }

// --- Float <-> double / integer conversions --------------------------------
__rcc_inline __m128 _mm_cvtpd_ps(__m128d __a) {
    return (__m128){(float)__a[0], (float)__a[1], 0.0f, 0.0f};
}
__rcc_inline __m128d _mm_cvtps_pd(__m128 __a) { return (__m128d){(double)__a[0], (double)__a[1]}; }
__rcc_inline __m128d _mm_cvtepi32_pd(__m128i __a) {
    __v4si_e __x = (__v4si_e)__a;
    return (__m128d){(double)__x[0], (double)__x[1]};
}
__rcc_inline __m128 _mm_cvtepi32_ps(__m128i __a) {
    __v4si_e __x = (__v4si_e)__a;
    return (__m128){(float)__x[0], (float)__x[1], (float)__x[2], (float)__x[3]};
}
__rcc_inline __m128i _mm_cvttpd_epi32(__m128d __a) {
    return (__m128i)(__v4si_e){(int)__a[0], (int)__a[1], 0, 0};
}
__rcc_inline __m128i _mm_cvttps_epi32(__m128 __a) {
    return (__m128i)(__v4si_e){(int)__a[0], (int)__a[1], (int)__a[2], (int)__a[3]};
}

// --- Pause / fences / cache control / streaming stores ---------------------
// <winnt.h> (via <windows.h>) references these through YieldProcessor,
// MemoryBarrier, LoadFence/StoreFence and CacheLineFlush; mingw's <intrin.h>
// only declares them, so the definitions must live here.
__rcc_inline void _mm_pause(void) { __asm__ __volatile__("pause"); }
__rcc_inline void _mm_lfence(void) { __asm__ __volatile__("lfence" ::: "memory"); }
__rcc_inline void _mm_mfence(void) { __asm__ __volatile__("mfence" ::: "memory"); }
__rcc_inline void _mm_clflush(const void *__p) {
    __asm__ __volatile__("clflush %0" : : "m"(*(const char *)__p) : "memory");
}
// Non-temporal stores: rcc has no movntpd/movntdq/movnti, so these lower to
// ordinary stores (dropping the non-temporal hint is semantically valid).
__rcc_inline void _mm_stream_pd(double *__p, __m128d __a) { *(__m128d *)__p = __a; }
__rcc_inline void _mm_stream_si128(__m128i *__p, __m128i __a) { *__p = __a; }
__rcc_inline void _mm_stream_si32(int *__p, int __a) { *__p = __a; }

#undef __rcc_inline
#endif // _EMMINTRIN_H_INCLUDED
