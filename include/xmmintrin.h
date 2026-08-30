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
// --- Square root / reciprocals (native sqrtps/rsqrtps builtin, no -lm) --------
__m128 __builtin_ia32_sqrtps(__m128);
__m128 __builtin_ia32_sqrtss(__m128);
__m128 __builtin_ia32_rsqrtps(__m128);
__rcc_inline __m128 _mm_sqrt_ps(__m128 __a) { return __builtin_ia32_sqrtps(__a); }
__rcc_inline __m128 _mm_sqrt_ss(__m128 __a) { return __builtin_ia32_sqrtss(__a); }
__rcc_inline __m128 _mm_rsqrt_ps(__m128 __a) { return __builtin_ia32_rsqrtps(__a); }

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
__rcc_inline __m128 _mm_move_ss(__m128 __a, __m128 __b) {
    // NOT `(__m128){__b[0], __a[1], __a[2], __a[3]}`: rcc keeps scalar
    // float values in GP registers as double-precision bit patterns, so
    // extracting `__a[1..3]`/`__b[0]` into GP registers and rebuilding
    // the vector round-trips each lane through cvtss2sd/cvtsd2ss. Per
    // the SSE spec those conversions MUST quiet a signaling NaN (set
    // the mantissa's top bit) and can shift its exact payload between
    // single/double precision's differently-positioned quiet bit --
    // silently corrupting any lane that happens to hold a NaN-shaped
    // bit pattern. SIMD code commonly reinterprets integer data as
    // `__m128` purely to use this blend as a cheap lane-merge (real
    // arithmetic never intended), so those "NaN" bit patterns are
    // actually meaningful integer payloads. A real `movss` between two
    // XMM registers merges the low lane of `__b` into `__a` (preserving
    // `__a`'s upper 3 lanes) as a pure bit copy, matching hardware
    // semantics exactly with no GP-register round trip at all.
    // Found via a real PHP build: Zend/hash_sha_sse2.c's SHA256 SSE2
    // transform uses this exact pattern (SPAN_ONE_THREE) to combine
    // message-schedule words; the corrupted payload propagated into the
    // hash state, breaking every hash()/hash_hmac() call project-wide
    // for a subset of message-schedule values.
    __asm__("movss %1, %0" : "+x"(__a) : "x"(__b));
    return __a;
}

// Minimal __m64 support: just enough to store the low/high 64 bits (two
// packed floats) of an __m128 through a `__m64 *` pointer, matching real
// GCC/Clang's <xmmintrin.h> _mm_storeh_pi/_mm_storel_pi signatures. This is
// NOT a general MMX implementation (no <mmintrin.h>, no __m64 arithmetic) --
// rcc has no need for MMX beyond this narrow float-pair-store use, seen in
// third-party SSE2 shuffle code that still spells its 64-bit float-pair
// stores this (pre-SSE2, `movlps`/`movhps`-era) way.
typedef long long __m64 __attribute__((__vector_size__(8), __may_alias__));
__rcc_inline void _mm_storel_pi(__m64 *__p, __m128 __a) {
    float *__fp = (float *)__p;
    __fp[0] = __a[0];
    __fp[1] = __a[1];
}
__rcc_inline void _mm_storeh_pi(__m64 *__p, __m128 __a) {
    float *__fp = (float *)__p;
    __fp[0] = __a[2];
    __fp[1] = __a[3];
}

// --- MMX-era SSE-extension intrinsics on __m64 -----------------------------
// A handful of `<xmmintrin.h>` intrinsics operate on __m64 (they were
// introduced alongside SSE to extend plain MMX): unsigned 16-bit multiply
// high, an 8-bit byte-sign movemask, and a 16-bit lane shuffle. Real GCC/
// Clang implement these via actual MMX registers/opcodes (pmulhuw, pmovmskb,
// pshufw); rcc has no MMX register class, so these are implemented as plain
// per-lane C operations over the same 8 bytes reinterpreted through a union
// -- identical results (this is exactly what the hardware opcodes compute),
// no MMX state (and so no _mm_empty()/EMMS pairing concern either). Needed
// by pixman's legacy MMX pixel-combiner fallback (pixman-mmx.c).
__rcc_inline __m64 _mm_mulhi_pu16(__m64 __A, __m64 __B) {
    union {
        __m64 v;
        unsigned short u[4];
    } __ua, __ub, __ur;
    __ua.v = __A;
    __ub.v = __B;
    for (int __i = 0; __i < 4; __i++)
        __ur.u[__i] = (unsigned short)(((unsigned)__ua.u[__i] * (unsigned)__ub.u[__i]) >> 16);
    return __ur.v;
}
__rcc_inline int _mm_movemask_pi8(__m64 __A) {
    union {
        __m64 v;
        unsigned char b[8];
    } __u;
    __u.v = __A;
    int __mask = 0;
    for (int __i = 0; __i < 8; __i++)
        __mask |= (__u.b[__i] >> 7) << __i;
    return __mask;
}
__rcc_inline __m64 _mm_shuffle_pi16(__m64 __A, int __N) {
    union {
        __m64 v;
        unsigned short u[4];
    } __ua, __ur;
    __ua.v = __A;
    __ur.u[0] = __ua.u[__N & 3];
    __ur.u[1] = __ua.u[(__N >> 2) & 3];
    __ur.u[2] = __ua.u[(__N >> 4) & 3];
    __ur.u[3] = __ua.u[(__N >> 6) & 3];
    return __ur.v;
}

// 4x4 single-precision transpose (SSE intrinsic macro, GCC/Clang-compatible).
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3)                              \
    do {                                                                       \
        __m128 __t0 = _mm_unpacklo_ps((row0), (row1));                         \
        __m128 __t1 = _mm_unpacklo_ps((row2), (row3));                         \
        __m128 __t2 = _mm_unpackhi_ps((row0), (row1));                         \
        __m128 __t3 = _mm_unpackhi_ps((row2), (row3));                         \
        (row0) = _mm_movelh_ps(__t0, __t1);                                    \
        (row1) = _mm_movehl_ps(__t1, __t0);                                    \
        (row2) = _mm_movelh_ps(__t2, __t3);                                    \
        (row3) = _mm_movehl_ps(__t3, __t2);                                    \
    } while (0)

// --- Conversions / extraction ----------------------------------------------
__rcc_inline float _mm_cvtss_f32(__m128 __a) { return __a[0]; }

// Scalar float<->int32 conversions. `_mm_cvtss_si32`/`_mm_cvtsi32_ss`
// are the modern names; `_mm_cvt_ss2si`/`_mm_cvt_si2ss` are the
// original (pre-2001) SSE intrinsic spelling real code (e.g. libopus)
// still uses -- both map to the same instruction. The codegen for
// these __builtin_ia32_* names already exists (cg_vectors.c's scalar
// int<->float conversion dispatch); only the header-level wrappers
// were missing.
int __builtin_ia32_cvtss2si(__m128);
int __builtin_ia32_cvttss2si(__m128);
__m128 __builtin_ia32_cvtsi2ss(__m128, int);
__rcc_inline int _mm_cvtss_si32(__m128 __a) { return __builtin_ia32_cvtss2si(__a); }
__rcc_inline int _mm_cvt_ss2si(__m128 __a) { return __builtin_ia32_cvtss2si(__a); }
__rcc_inline int _mm_cvttss_si32(__m128 __a) { return __builtin_ia32_cvttss2si(__a); }
__rcc_inline int _mm_cvtt_ss2si(__m128 __a) { return __builtin_ia32_cvttss2si(__a); }
__rcc_inline __m128 _mm_cvtsi32_ss(__m128 __a, int __b) { return __builtin_ia32_cvtsi2ss(__a, __b); }
__rcc_inline __m128 _mm_cvt_si2ss(__m128 __a, int __b) { return __builtin_ia32_cvtsi2ss(__a, __b); }
#if defined(__x86_64__) || defined(_M_X64)
long long __builtin_ia32_cvtss2si64(__m128);
long long __builtin_ia32_cvttss2si64(__m128);
__m128 __builtin_ia32_cvtsi642ss(__m128, long long);
__rcc_inline long long _mm_cvtss_si64(__m128 __a) { return __builtin_ia32_cvtss2si64(__a); }
__rcc_inline long long _mm_cvttss_si64(__m128 __a) { return __builtin_ia32_cvttss2si64(__a); }
__rcc_inline __m128 _mm_cvtsi64_ss(__m128 __a, long long __b) {
    return __builtin_ia32_cvtsi642ss(__a, __b);
}
#endif

__rcc_inline int _mm_movemask_ps(__m128 __a) {
    union {
        __m128 __v;
        unsigned __u[4];
    } __m;
    __m.__v = __a;
    return (int)((__m.__u[0] >> 31) | ((__m.__u[1] >> 31) << 1) |
                 ((__m.__u[2] >> 31) << 2) | ((__m.__u[3] >> 31) << 3));
}

// --- Prefetch / fences / streaming stores ----------------------------------
// <winnt.h> (via <windows.h>) references these through StoreFence/_mm_sfence
// and PreFetchCacheLine; mingw's <intrin.h> only declares them, so the
// definitions must live here. _mm_getcsr/_mm_setcsr are intentionally
// left to mingw's own extern declaration/runtime on that target; on
// other targets they'd need STMXCSR/LDMXCSR (now supported, see
// __builtin_ia32_stmxcsr/__builtin_ia32_ldmxcsr and inline-asm), not
// wired up here as no third-party target has needed them yet.
#ifndef _MM_HINT_T0
#define _MM_HINT_T0 3
#define _MM_HINT_T1 2
#define _MM_HINT_T2 1
#define _MM_HINT_NTA 0
#endif
__rcc_inline void _mm_prefetch(const void *__p, int __sel) {
    (void)__sel;
    __builtin_prefetch(__p);
}
__rcc_inline void _mm_sfence(void) { __asm__ __volatile__("sfence" ::: "memory"); }
// Non-temporal stores: rcc has no movntps, so these lower to ordinary stores
// (the non-temporal hint is dropped, which is semantically valid).
__rcc_inline void _mm_stream_ps(float *__p, __m128 __a) { *(__m128 *)__p = __a; }

// Compare predicates for scalar/packed compares (the real headers' AVX-512
// compare intrinsics reference these by name).
#define _CMP_EQ_OQ 0x00
#define _CMP_LT_OS 0x01
#define _CMP_LE_OS 0x02
#define _CMP_UNORD_Q 0x03
#define _CMP_NEQ_UQ 0x04
#define _CMP_NLT_US 0x05
#define _CMP_NLE_US 0x06
#define _CMP_ORD_Q 0x07
#define _CMP_EQ_UQ 0x08
#define _CMP_NGE_US 0x09
#define _CMP_NGT_US 0x0a
#define _CMP_FALSE_OQ 0x0b
#define _CMP_NEQ_OQ 0x0c
#define _CMP_GE_OS 0x0d
#define _CMP_GT_OS 0x0e
#define _CMP_TRUE_UQ 0x0f
#define _CMP_EQ_OS 0x10
#define _CMP_LT_OQ 0x11
#define _CMP_LE_OQ 0x12
#define _CMP_UNORD_S 0x13
#define _CMP_NEQ_US 0x14
#define _CMP_NLT_UQ 0x15
#define _CMP_NLE_UQ 0x16
#define _CMP_ORD_S 0x17
#define _CMP_EQ_US 0x18
#define _CMP_NGE_UQ 0x19
#define _CMP_NGT_UQ 0x1a
#define _CMP_FALSE_OS 0x1b
#define _CMP_NEQ_OS 0x1c
#define _CMP_GE_OQ 0x1d
#define _CMP_GT_OQ 0x1e
#define _CMP_TRUE_US 0x1f

#undef __rcc_inline

// Real GCC/Clang's <xmmintrin.h> auto-chains to <emmintrin.h> once SSE2 is
// enabled (the default on x86-64) -- third-party code commonly includes
// only <xmmintrin.h> and then freely uses SSE2-only intrinsics
// (_mm_storeu_si128/_mm_loadu_si128/__m128i/...), relying on that chain
// rather than including <emmintrin.h> itself. Safe against the reciprocal
// `#include <xmmintrin.h>` at the top of emmintrin.h: both headers guard
// their own body with an include-guard macro.
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#endif // _XMMINTRIN_H_INCLUDED
