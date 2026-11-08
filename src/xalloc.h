// SPDX-License-Identifier: LGPL-2.1-or-later
// malloc/calloc/realloc that never return NULL: on failure they print
// "rcc: out of memory" and exit(1) (alloc.c). A zero size is bumped to 1, so
// the result is always a valid, freeable pointer and a failed realloc cannot
// leak or null the caller's buffer.
#ifndef XALLOC_H
#define XALLOC_H

#include <stddef.h>

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t sz);
void *xrealloc(void *p, size_t n);

char *xstrdup(const char *s);
#endif
