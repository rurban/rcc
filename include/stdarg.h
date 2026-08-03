#ifndef RCC_STDARG_H
#define RCC_STDARG_H

typedef __builtin_va_list va_list;

/* glibc headers (stdio.h, wchar.h, ...) reference __gnuc_va_list, which the
 * compiler is expected to provide. gcc defines it from <stdarg.h>; mirror that
 * so the system headers parse when they #include <stdarg.h>. */
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef __builtin_va_list __gnuc_va_list;
#endif
/* C23: va_start takes the va_list plus an optional last-named-parameter
 * argument; any further arguments are permitted but not evaluated. */
#define va_start(...)      __builtin_va_start(__VA_ARGS__)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDARG_H__ 202311L
#endif

#endif
