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
#define LONG_MIN (-2147483647L - 1L)
#define LONG_MAX 2147483647L
#define ULONG_MAX 0xffffffffUL
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
#define LONG_WIDTH 64
#define ULONG_WIDTH 64
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
#ifndef _LIBC_LIMITS_H_
#include_next <limits.h>
#endif

#endif
