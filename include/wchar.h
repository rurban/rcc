#ifndef RCC_WCHAR_H
#define RCC_WCHAR_H

#include <stddef.h>

/* Pull in the platform <wchar.h> for mbstate_t, wint_t, and the
   wide-character / multibyte conversion functions (mbrlen, mbrtowc,
   wcrtomb, ...). rcc's bundled copy is intentionally thin; everything
   real lives in the system header. */
#include_next <wchar.h>

#ifndef WEOF
#define WEOF (-1)
#endif

#endif
