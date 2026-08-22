#if !defined(RCC_STDDEF_H) || defined(__need_size_t) || defined(__need_NULL) || defined(__need_nullptr_t) || defined(__need_wchar_t) || defined(__need_ptrdiff_t)

#ifndef RCC_STDDEF_H
#define RCC_STDDEF_H
/* See include/stdint.h for why: this bundled header can also be the
 * first standard header a TU includes, so it must trigger the same
 * <features.h> feature-macro cascade (__GLIBC__ et al) real glibc
 * headers would. Macro-only header, no type conflicts. */
#ifdef __linux__
#include <features.h>
#endif
#endif

#ifndef __need_ptrdiff_t
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif
#ifdef __need_wchar_t
/* glibc headers (e.g. <stdlib.h>) request just wchar_t via
 * `#define __need_wchar_t` + `#include <stddef.h>`; that path must
 * PROVIDE the typedef, not skip it (the old `#ifndef` inverted this,
 * leaving wchar_t undefined whenever glibc's real stdlib.h was used,
 * e.g. with `-I/usr/include`). */
typedef __WCHAR_TYPE__ wchar_t;
#else
typedef __WCHAR_TYPE__ wchar_t;
#endif
typedef __SIZE_TYPE__ size_t;
typedef __SSIZE_TYPE__ ssize_t;

typedef struct {
    long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
    long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

#if __STDC_VERSION__ >= 202311L
typedef typeof(nullptr) nullptr_t;
#define __STDC_VERSION_STDDEF_H__ 202311L
#else
typedef void *nullptr_t;
#endif

#ifndef __need_NULL
#define NULL ((void *)0)
#endif
#define offsetof(type, member) __builtin_offsetof(type, member)

#if __STDC_VERSION__ >= 202311L
#define unreachable() __builtin_unreachable()
#endif

#endif
