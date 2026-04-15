#ifndef RCC_STDARG_H
#define RCC_STDARG_H

typedef char *va_list;
#define va_start(ap, last)
#define va_end(ap)
#define va_arg(ap, type) (*(type*)0)
#define va_copy(dest, src) ((dest) = (src))

#endif
