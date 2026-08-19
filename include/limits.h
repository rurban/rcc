#ifndef RCC_LIMITS_H
#define RCC_LIMITS_H

#define CHAR_BIT 8
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define CHAR_MIN (-128)
#define CHAR_MAX 127
#define SHRT_MIN (-32768)
#define SHRT_MAX 32767
#define USHRT_MAX 65535
#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define UINT_MAX 0xffffffffU
/* `long` is 8 bytes on LP64 targets (Linux/macOS x86-64 and ARM64) but
 * only 4 bytes on LLP64 Windows/mingw, even in 64-bit builds -- unlike
 * every other macro in this file, these three are NOT portable
 * constants and must reflect the real target ABI. __LONG_MAX__ /
 * __SIZEOF_LONG__ come from gcc_predefined.h (baked in at build time
 * from the real target gcc's own -dM -E dump), so they already carry
 * the correct per-target value; derive from them instead of hardcoding
 * the 32-bit case unconditionally. A stale/wrong LONG_MAX here is not
 * just cosmetically wrong -- any header included later in the same TU
 * that computes something FROM LONG_MAX's value at that point in the
 * include chain (e.g. glibc's <bits/xopen_lim.h> deriving LONG_BIT)
 * bakes in the wrong answer permanently, even though a subsequent
 * `#include_next` below eventually redefines LONG_MAX itself correctly
 * -- found via CPython's pyport.h, which detects exactly this
 * inconsistency and refuses to compile rather than silently
 * miscompiling. */
#define LONG_MAX __LONG_MAX__
#define LONG_MIN (-LONG_MAX - 1L)
#if __SIZEOF_LONG__ == 8
#define ULONG_MAX 0xffffffffffffffffUL
#else
#define ULONG_MAX 0xffffffffUL
#endif
#define LLONG_MIN (-9223372036854775807LL - 1LL)
#define LLONG_MAX 9223372036854775807LL
#define ULLONG_MAX 18446744073709551615ULL
#ifndef SSIZE_MAX
#if __SIZEOF_POINTER__ == 8
#define SSIZE_MAX 9223372036854775807LL
#else
#define SSIZE_MAX 2147483647
#endif
#endif

#define MB_LEN_MAX 16

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_LIMITS_H__ 202311L

/* C23 width macros */
#define CHAR_WIDTH 8
#define SCHAR_WIDTH 8
#define UCHAR_WIDTH 8
#define SHRT_WIDTH 16
#define USHRT_WIDTH 16
#define INT_WIDTH 32
#define UINT_WIDTH 32
/* __LONG_WIDTH__ (from gcc_predefined.h): 64 on LP64, 32 on LLP64
 * Windows/mingw -- same ABI distinction as LONG_MAX above. */
#define LONG_WIDTH __LONG_WIDTH__
#define ULONG_WIDTH __LONG_WIDTH__
#define LLONG_WIDTH 64
#define ULLONG_WIDTH 64

/* C23 bool limits */
#define BOOL_MAX 1
#define BOOL_WIDTH 1
#endif

/* Chain onward to the platform's real <limits.h> (glibc's, on Linux):
 * this header only defines the ISO C minimums above plus SSIZE_MAX,
 * and must not shadow the system one. POSIX/XSI macros like PIPE_BUF,
 * NL_ARGMAX and platform internals such as __WORDSIZE live in the
 * system header. Same pattern as GCC's own fixed-include limits.h,
 * which ends in `#include_next <limits.h>`. rcc's RCC_LIMITS_H guard
 * keeps the recursion bounded (glibc defines _LIBC_LIMITS_H_ itself
 * and never re-includes us).
 */
#if defined(__GLIBC__) && !defined(_LIBC_LIMITS_H_)
#include_next <limits.h>
#endif

/* Linux kernel limits (PATH_MAX, PIPE_BUF, etc.) and POSIX minimum
 * values (_POSIX_ARG_MAX, _POSIX_PATH_MAX, etc.). Always define these
 * on Linux regardless of glibc detection, since many projects expect
 * them unconditionally from <limits.h>.
 *
 * Defined directly rather than including <linux/limits.h> because musl's
 * sysroot lacks that header. Values are identical to the kernel header. */
#ifdef __linux__
#ifndef PATH_MAX
#define PATH_MAX        4096
#endif
#ifndef PIPE_BUF
#define PIPE_BUF        4096
#endif
#ifndef NAME_MAX
#define NAME_MAX        255
#endif
#ifndef LINK_MAX
#define LINK_MAX        127
#endif
#ifndef MAX_CANON
#define MAX_CANON       255
#endif
#ifndef MAX_INPUT
#define MAX_INPUT       255
#endif
#ifndef ARG_MAX
#define ARG_MAX         131072
#endif
#ifndef NGROUPS_MAX
#define NGROUPS_MAX     65536
#endif
#ifndef NR_OPEN
#define NR_OPEN         1024
#endif
/* POSIX minimum values — standard-mandated lower bounds. */
#ifndef _POSIX_ARG_MAX
#define _POSIX_ARG_MAX 4096
#endif
#ifndef _POSIX_CHILD_MAX
#define _POSIX_CHILD_MAX 25
#endif
#ifndef _POSIX_LINK_MAX
#define _POSIX_LINK_MAX 8
#endif
#ifndef _POSIX_MAX_CANON
#define _POSIX_MAX_CANON 255
#endif
#ifndef _POSIX_MAX_INPUT
#define _POSIX_MAX_INPUT 255
#endif
#ifndef _POSIX_NAME_MAX
#define _POSIX_NAME_MAX 14
#endif
#ifndef _POSIX_NGROUPS_MAX
#define _POSIX_NGROUPS_MAX 0
#endif
#ifndef _POSIX_OPEN_MAX
#define _POSIX_OPEN_MAX 20
#endif
#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256
#endif
#ifndef _POSIX_PIPE_BUF
#define _POSIX_PIPE_BUF 512
#endif
#ifndef _POSIX_SSIZE_MAX
#define _POSIX_SSIZE_MAX 32767
#endif
#ifndef _POSIX_STREAM_MAX
#define _POSIX_STREAM_MAX 8
#endif
#ifndef _POSIX_TZNAME_MAX
#define _POSIX_TZNAME_MAX 6
#endif
/* IOV_MAX — maximum number of iovec entries for readv/writev.
 * Defined in glibc's <bits/xopen_lim.h> only when __USE_XOPEN is set,
 * but many projects expect it unconditionally from <limits.h>. */
#ifndef IOV_MAX
#define IOV_MAX 1024
#endif
#endif
#endif

#endif
