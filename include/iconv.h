#ifndef _RCC_ICONV_H
#define _RCC_ICONV_H

#include <stddef.h>

#if defined(_WIN32) || defined(__APPLE__)
// Neither target reliably ships a native <iconv.h> to chain onward to
// (Windows has none at all; macOS's is libiconv-provided and only
// present when Xcode's command-line tools / a package manager installed
// it) -- stay self-contained here, matching the signatures both glibc's
// and libiconv's real headers use.
typedef void *iconv_t;

iconv_t iconv_open(const char *tocode, const char *fromcode);
size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
             char **outbuf, size_t *outbytesleft);
int iconv_close(iconv_t cd);
#else
// Pull in the platform <iconv.h>: mirrors stdio.h/wchar.h/limits.h's own
// include_next pattern (see stdio.h's comment) -- a thin bundled shadow
// silently drops whatever else the real header (or a project's own -I
// override reached through it, e.g. ast/ksh93's ast_iconv.h) provides.
// Signatures match exactly (verified against glibc's <iconv.h>), so no
// declaration conflicts once this chains onward.
#include_next <iconv.h>
#endif

#endif
