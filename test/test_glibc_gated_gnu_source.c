/* A common real-world idiom (e.g. dyncall's dynload_syms_elf.c, pulled
 * in by nqp/MoarVM): a TU includes some system header first (here
 * <elf.h> or <stdint.h>/<stddef.h> directly), then gates its own
 * `#define _GNU_SOURCE` / `#define __USE_GNU` behind `#if defined
 * (__GLIBC__)` -- since <features.h> is include-guarded, this only
 * works if __GLIBC__ became visible from that FIRST system header.
 * <dlfcn.h>'s Dl_info/dladdr are themselves gated on __USE_GNU, so
 * this exercises the whole chain end to end.
 *
 * rcc's own bundled <stdint.h>/<stddef.h> shadow glibc's (by include
 * search order), and used to never pull in glibc's real <features.h>,
 * so __GLIBC__ stayed permanently undefined for the whole TU --
 * silently skipping the `#if defined(__GLIBC__)` block, and thus
 * <dlfcn.h>'s Dl_info/dladdr declarations too, unlike real gcc which
 * always sees __GLIBC__ via glibc's own <stdint.h>/<stddef.h>.
 * found via test/third_party/test_nqp (dyncall's dynload_syms_elf.c).
 */
#include <stdint.h>

#if defined(__GLIBC__)
#define _GNU_SOURCE
#define __USE_GNU
#endif

#include <dlfcn.h>
#include <stdio.h>

const char *name_from_value(void *value) {
#ifdef __USE_GNU
    Dl_info info;
    return (dladdr(value, &info) && (value == info.dli_saddr)) ? info.dli_sname : "?";
#else
    (void)value;
    return "?";
#endif
}

int main(void) {
#ifndef __linux__
    printf("OK\n");
    return 0;
#else
    const char *n = name_from_value((void *)&main);
    if (!n) {
        printf("FAIL: dladdr/Dl_info chain broken\n");
        return 1;
    }
    printf("OK\n");
    return 0;
#endif
}
