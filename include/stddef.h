#if !defined(RCC_STDDEF_H) || defined(__need_size_t) || defined(__need_NULL) || defined(__need_nullptr_t) || defined(__need_wchar_t) || defined(__need_ptrdiff_t)

#ifndef RCC_STDDEF_H
#define RCC_STDDEF_H
#endif

#ifndef __need_ptrdiff_t
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif
#ifndef __need_wchar_t
typedef __WCHAR_TYPE__ wchar_t;
#endif
typedef __SIZE_TYPE__ size_t;

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
