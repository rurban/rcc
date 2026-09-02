#ifndef RCC_LIMITS_H
#define RCC_LIMITS_H

#define CHAR_BIT 8
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#ifdef _WIN32
#define CHAR_MIN 0
#define CHAR_MAX UCHAR_MAX
#else
#define CHAR_MIN (-128)
#define CHAR_MAX 127
#endif // _WIN32
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
#ifdef _WIN32
#define LONG_MAX 2147483647L
#define LONG_MIN (-LONG_MAX - 1L)
#define ULONG_MAX 0xffffffffUL
#else
#if __SIZEOF_LONG__ == 8
#define ULONG_MAX 0xffffffffffffffffUL
#else
#define ULONG_MAX 0xffffffffUL
#endif
#endif // !_WIN32
#define LLONG_MIN (-9223372036854775807LL - 1LL)
#define LLONG_MAX 9223372036854775807LL
#define ULLONG_MAX 18446744073709551615ULL
#ifndef SSIZE_MAX
#if __SIZEOF_POINTER__ == 8
#define SSIZE_MAX 9223372036854775807LL
#else
#define SSIZE_MAX 2147483647
#endif
#endif // SSIZE_MAX

#ifdef _WIN32
#define MB_LEN_MAX 5
#else
#define MB_LEN_MAX 16
#endif

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
#endif // C23

/* All limits (ISO C, POSIX/XSI, Linux kernel, GNU extensions) are defined
 * directly in this header. No need to chain to the platform's real
 * <limits.h> — rcc's own definitions cover everything projects need. */

/* Linux kernel limits (PATH_MAX, PIPE_BUF, etc.) and POSIX minimum
 * values (_POSIX_ARG_MAX, _POSIX_PATH_MAX, etc.). Always define these
 * on Linux regardless of glibc detection, since many projects expect
 * them unconditionally from <limits.h>.
 *
 * Defined directly rather than including <linux/limits.h> because musl's
 * sysroot lacks that header. Values are identical to the kernel header. */
#ifdef __linux__
#ifndef PATH_MAX
#ifdef _WIN32
#define PATH_MAX        260
#else
#define PATH_MAX        4096
#endif
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
/* PTHREAD_STACK_MIN — minimum thread stack size (POSIX 2008).
 * Defined in glibc's <bits/posix2_lim.h> only when __USE_POSIX2 is set,
 * but many projects expect it unconditionally from <limits.h>. */
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif
/* POSIX/XSI and GNU extensions — define all commonly used limits
 * unconditionally so projects don't need _GNU_SOURCE or _POSIX_C_SOURCE. */
#ifndef AIO_PRIO_DELTA_MAX
#define AIO_PRIO_DELTA_MAX 20
#endif
#ifndef BC_BASE_MAX
#define BC_BASE_MAX 99
#endif
#ifndef BC_DIM_MAX
#define BC_DIM_MAX 2048
#endif
#ifndef BC_SCALE_MAX
#define BC_SCALE_MAX 99
#endif
#ifndef BC_STRING_MAX
#define BC_STRING_MAX 1000
#endif
#ifndef CHARCLASS_NAME_MAX
#define CHARCLASS_NAME_MAX 2048
#endif
#ifndef COLL_WEIGHTS_MAX
#define COLL_WEIGHTS_MAX 255
#endif
#ifndef DELAYTIMER_MAX
#define DELAYTIMER_MAX 2147483647
#endif
#ifndef EXPR_NEST_MAX
#define EXPR_NEST_MAX 256
#endif
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif
#ifndef LINE_MAX
#define LINE_MAX 2048
#endif
#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif
#ifndef MQ_PRIO_MAX
#define MQ_PRIO_MAX 32768
#endif
#ifndef PTHREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_DESTRUCTOR_ITERATIONS 4
#endif
#ifndef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX 1024
#endif
#ifndef RE_DUP_MAX
#define RE_DUP_MAX 32767
#endif
#ifndef RTSIG_MAX
#define RTSIG_MAX 32
#endif
#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX 2147483647
#endif
#ifndef TTY_NAME_MAX
#define TTY_NAME_MAX 32
#endif
#ifndef XATTR_LIST_MAX
#define XATTR_LIST_MAX 65536
#endif
#ifndef XATTR_NAME_MAX
#define XATTR_NAME_MAX 255
#endif
#ifndef XATTR_SIZE_MAX
#define XATTR_SIZE_MAX 65536
#endif
#ifndef BITINT_MAXWIDTH
#define BITINT_MAXWIDTH __BITINT_MAXWIDTH__
#endif
/* SIZE_MAX — maximum value of size_t (C11 7.20.3). */
#ifndef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#endif
/* LONG_BIT — number of bits in a long (POSIX, used by yash, git, etc.).
 * Defined in glibc's <bits/xopen_lim.h>; many projects expect it from
 * <limits.h>. */
#ifndef LONG_BIT
#define LONG_BIT (__SIZEOF_LONG__ * 8)
#endif
#endif

/* Chain to the platform's real <limits.h> for the feature-macro-guarded
 * content this bundled copy doesn't track: glibc's _POSIX2_* / _XOPEN_* /
 * _SC_* limits (bits/posix2_lim.h, bits/xopen_lim.h) used by getconf-
 * style code. rcc's own INT_MAX et al. above stay put; glibc's own
 * trailing `#include_next` is disabled by the predefined _GCC_LIMITS_H_.
 * Linux-only: mingw's limits.h transitively defines the C23 `thread_local`
 * keyword as a macro, which broke file-scope thread_local compound
 * literals (torture/c23-complit-5.c) on Windows. */
#ifdef __linux__
#include_next <limits.h>
#endif

#endif // RCC_LIMITS_H
