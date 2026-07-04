// SPDX-License-Identifier: LGPL-2.1-or-later
// Native PE/COFF linker for rcc (stub: falls back to external linker).
#include "link.h"

int link_load_object(LinkState *s, const char *path) {
    (void)s;
    (void)path;
    return -1;
}

int link_pe(LinkState *s) {
    (void)s;
    return -1;
}
