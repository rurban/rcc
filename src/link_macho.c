// SPDX-License-Identifier: LGPL-2.1-or-later
// Native Mach-O 64-bit linker for rcc (stub: falls back to external linker).
#include "link.h"

int link_load_object(LinkState *s, const char *path) {
    (void)s;
    (void)path;
    return -1;
}

int link_macho(LinkState *s) {
    (void)s;
    return -1;
}
