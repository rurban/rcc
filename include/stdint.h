#ifndef RCC_STDINT_H
#define RCC_STDINT_H

/* This bundled header shadows glibc's own <stdint.h> -- the header
 * virtually every non-trivial C translation unit includes first.
 * Real gcc's <stdint.h> transitively pulls in glibc's <features.h>,
 * which defines __GLIBC__/__USE_GNU/etc; a lot of real-world code
 * gates GNU-extension declarations (e.g. <dlfcn.h>'s Dl_info/dladdr)
 * behind `#if defined(__GLIBC__)`. Without this, that check silently
 * evaluates false for the whole TU since our own headers never
 * trigger it. <features.h> declares only feature-test macros (plus
 * <sys/cdefs.h>/<gnu/stubs.h>, likewise macro-only) -- no types -- so
 * pulling it in here cannot conflict with our own typedefs below.
 * found via test/third_party/test_nqp: dyncall's dynload_syms_elf.c
 * (`#if defined(__GLIBC__) #define _GNU_SOURCE ...`) never saw
 * Dl_info/dladdr declared in <dlfcn.h>. */
#ifdef __linux__
#include <features.h>
#endif

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
// On LP64 targets (native Linux/macOS x86-64 and arm64, where `long` is
// 8 bytes), these must be `long`/`unsigned long`, matching glibc's own
// convention (bits/types.h's __int64_t/__intmax_t, guarded on
// __WORDSIZE == 64) -- not merely "some 64-bit type", which the C
// standard alone would allow. A typedef name that's re-typedef'd to a
// DIFFERENT (if same-size) underlying type is not the same type per
// C11 6.7p3, so any TU that pulls in both this header and a real
// glibc header defining these names again (extremely common --
// countless system headers indirectly include <bits/stdint-intn.h>
// for __intN_t) hit a real "conflicting types" error at every
// int64_t/intmax_t/intptr_t-parametered function once the parser
// started diagnosing incompatible redeclarations -- the exact same
// class of bug the ptrdiff_t note below already documents, just for
// four more names. On LLP64 (Windows/mingw, where `long` is only 4
// bytes), `long long` remains correct and matches MSVC/mingw's own
// convention there.
// found via test/third_party/test_libtommath: MP_INIT_INT(mp_init_i64,
// mp_set_i64, int64_t)'s macro-expanded definition disagreed with its
// own header-declared prototype once glibc's <bits/types.h> (pulled in
// transitively) re-typedef'd int64_t as `long` right after this
// header's own `long long` typedef had already taken effect.
#ifdef _WIN32
typedef long long int64_t;
typedef unsigned long long uint64_t;
#else
typedef long int64_t;
typedef unsigned long uint64_t;
#endif

#ifdef _WIN32
typedef long long intptr_t;
typedef unsigned long long uintptr_t;
#else
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif

#ifdef _WIN32
typedef long long intmax_t;
typedef unsigned long long uintmax_t;
#else
typedef long intmax_t;
typedef unsigned long uintmax_t;
#endif

/* NOTE: ptrdiff_t belongs in <stddef.h>, not here -- real glibc's
 * <stdint.h> does not define it. A stray `typedef long long ptrdiff_t;`
 * used to live here, redeclaring (with the WRONG underlying type: long
 * long instead of stddef.h's correct long int) whatever <stddef.h> had
 * already typedef'd if both headers were included -- previously
 * harmless only because nothing validated the two typedefs agreed;
 * once the parser started diagnosing incompatible redeclarations, any
 * TU including both headers (e.g. zfp's C "template" sources, via
 * <stdint.h> after <stddef.h>) hit a real "conflicting types" error at
 * every ptrdiff_t-parametered function definition. */
/* Minimum-width integer types (C99 7.18.1.2) */
typedef int8_t int_least8_t;
typedef uint8_t uint_least8_t;
typedef int16_t int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;
typedef int64_t int_least64_t;
typedef uint64_t uint_least64_t;

/* Fastest minimum-width integer types (C99 7.18.1.3) */
typedef int8_t int_fast8_t;
typedef uint8_t uint_fast8_t;
typedef int16_t int_fast16_t;
typedef uint16_t uint_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;
typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t;

#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255
#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 65535
#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 0xffffffffU
// INT64_MIN/MAX/UINT64_MAX's literal suffix must match int64_t/uint64_t's
// actual typedef'd type (see the _WIN32-conditional typedefs above): `long`/
// `unsigned long` on LP64 (Linux/macOS), `long long`/`unsigned long long` on
// LLP64 (Windows/mingw, where `long` is only 4 bytes). A mismatched suffix
// (e.g. always `LL`/`ULL`) makes the macro's own type disagree with
// int64_t/uint64_t/intmax_t/uintmax_t/size_t -- caught by gnulib's own
// "does stdint.h conform to C99" configure probe: `_Generic (SIZE_MAX,
// size_t: 0)` found no matching association, since SIZE_MAX (built from
// UINT64_MAX) was `unsigned long long` while size_t is `unsigned long` on
// this LP64 target. This mismatch previously made gnulib substitute its own
// (partly UB-laden) <stdint.h> replacement for every rcc-built project.
#ifdef _WIN32
#define INT64_MIN (-9223372036854775807LL - 1LL)
#define INT64_MAX 9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL
#else
#define INT64_MIN (-9223372036854775807L - 1L)
#define INT64_MAX 9223372036854775807L
#define UINT64_MAX 18446744073709551615UL
#endif

#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX

#define SIZE_MAX UINT64_MAX
#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

/* Minimum-width integer limit macros (C99 7.18.2.2) */
#define INT_LEAST8_MIN  INT8_MIN
#define INT_LEAST8_MAX  INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* Fastest minimum-width integer limit macros (C99 7.18.2.3) */
#define INT_FAST8_MIN  INT8_MIN
#define INT_FAST8_MAX  INT8_MAX
#define UINT_FAST8_MAX UINT8_MAX
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST16_MAX INT16_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX


/* Integer constant macros (C99 7.18.4.1) */
#define INT8_C(v)  v
#define UINT8_C(v) v
#define INT16_C(v) v
#define UINT16_C(v) v
#define INT32_C(v) v
#define UINT32_C(v) v ## U
#define INT64_C(v) v ## LL
#define UINT64_C(v) v ## ULL
#define INTMAX_C(v) v ## LL
#define UINTMAX_C(v) v ## ULL

/* Limits of wchar_t/wint_t/sig_atomic_t (C99 7.18.3), required in
 * <stdint.h> -- previously missing entirely, which fails gnulib's own
 * "does stdint.h conform to C99" configure probe (its conftest checks
 * `#ifdef WCHAR_MIN ... #ifdef WCHAR_MAX`), causing gnulib to
 * conservatively substitute its own <stdint.h> replacement for every
 * project built with rcc, unlike the identical build with GCC/clang.
 * Deliberately NOT derived from __WCHAR_MIN__/__WCHAR_MAX__ etc.: those
 * are regenerated per-target from the system compiler
 * (src/gcc_predefined.h) and Apple's Clang (macOS's system "gcc")
 * doesn't reliably define the full set the way GCC does. These values
 * are fixed for every rcc target instead: wchar_t is `int` (32-bit
 * signed) on Linux/macOS, `unsigned short` (16-bit) on Windows/mingw;
 * wint_t is `unsigned int` and sig_atomic_t is `int` (32-bit) on every
 * target. */
#ifdef _WIN32
#define WCHAR_MIN 0
#define WCHAR_MAX 0xffff
#else
#define WCHAR_MIN (-2147483647 - 1)
#define WCHAR_MAX 2147483647
#endif
#define WINT_MIN 0U
#define WINT_MAX 0xffffffffU
#define SIG_ATOMIC_MIN (-2147483647 - 1)
#define SIG_ATOMIC_MAX 2147483647


/* Width macros (C23 7.20.3). diffutils' lib/io.c uses SIZE_WIDTH and
 * PTRDIFF_WIDTH; the general set covers every type with a limit macro. */
#define INTPTR_WIDTH  64
#define UINTPTR_WIDTH 64
#define SIZE_WIDTH    64
#define PTRDIFF_WIDTH 64
#define WCHAR_WIDTH   32
#define WINT_WIDTH    32
#define SIG_ATOMIC_WIDTH 32
#define INTMAX_WIDTH  64
#define UINTMAX_WIDTH 64
#define INT8_WIDTH    8
#define UINT8_WIDTH   8
#define INT16_WIDTH   16
#define UINT16_WIDTH  16
#define INT32_WIDTH   32
#define UINT32_WIDTH  32
#define INT64_WIDTH   64
#define UINT64_WIDTH  64
#define INT_LEAST8_WIDTH  8
#define UINT_LEAST8_WIDTH 8
#define INT_LEAST16_WIDTH 16
#define UINT_LEAST16_WIDTH 16
#define INT_LEAST32_WIDTH 32
#define UINT_LEAST32_WIDTH 32
#define INT_LEAST64_WIDTH 64
#define UINT_LEAST64_WIDTH 64
#define INT_FAST8_WIDTH  8
#define UINT_FAST8_WIDTH 8
#define INT_FAST16_WIDTH 16
#define UINT_FAST16_WIDTH 16
#define INT_FAST32_WIDTH 32
#define UINT_FAST32_WIDTH 32
#define INT_FAST64_WIDTH 64
#define UINT_FAST64_WIDTH 64
#if __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDINT_H__ 202311L
#endif
#endif
