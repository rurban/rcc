// SPDX-License-Identifier: LGPL-2.1-or-later
// Regression test: rcc's native ELF linker's section-alignment lookup
// read the wrong Elf64_Shdr field (sh_size at offset 32 instead of
// sh_addralign at offset 48 -- see sec_alignment() in link_elf.c). For a
// multi-object link, that fed the byte-length of a later input file's
// section into align_up()'s power-of-2-only bitmask trick as if it were
// its alignment; whenever that length wasn't already a power of 2 (the
// overwhelmingly common case), the bitmask computed garbage padding
// instead of a genuine round-up, splicing spurious zero bytes into the
// middle of the merged section.
//
// For `.init_array` specifically, those zero bytes are zero-valued
// constructor-pointer slots: the CRT startup code calls every entry in
// `.init_array` unconditionally before main() runs, so a spliced zero
// entry is a jump to address 0 -- SIGSEGV before the program's own code
// ever executes. Found via a real third-party build (LZ4's constructor-
// registering test harness, HandmadeMath's COVERAGE-macro constructors
// split across two .o files): 1 file's constructors (127 real entries)
// followed by 4 zero (NULL) entries followed by another file's 5 real
// entries, crashing on the first NULL call.
//
// This test reproduces the minimal case: linking two separate
// translation units, each contributing `__attribute__((constructor))`
// functions to the merged `.init_array`, with counts chosen so the buggy
// alignment computation would splice a zero entry between them (1
// constructor in the first-linked object, 2 in the second -- see the
// fix's commit message for the exact arithmetic). Every constructor
// increments a global counter with a unique bit; main() checks the
// bitmask is exactly right. A regression manifests as a segfault before
// main() ever runs (the zero-entry call), not a wrong counter value --
// this test's real assertion is simply "the program didn't crash and
// every constructor actually ran".
#define _DEFAULT_SOURCE
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32) && !defined(__APPLE__)

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char buf[1024];

    char src1[512], src2[512], srcmain[512], exe[512];
    snprintf(src1, sizeof(src1), "%s/test_lco_a_%d.c", td, pid);
    snprintf(src2, sizeof(src2), "%s/test_lco_b_%d.c", td, pid);
    snprintf(srcmain, sizeof(srcmain), "%s/test_lco_main_%d.c", td, pid);
    snprintf(exe, sizeof(exe), "%s/test_lco_exe_%d", td, pid);

    // First-linked object: exactly 1 constructor.
    FILE *f1 = fopen(src1, "w");
    if (!f1) { printf("FAIL: cannot write %s\n", src1); return 1; }
    fputs(
        "extern int g_ctor_mask;\n"
        "static void __attribute__((constructor)) ctor_a1(void) { g_ctor_mask |= 1; }\n",
        f1);
    fclose(f1);

    // Second-linked object: exactly 2 constructors, plus the shared
    // definition of g_ctor_mask (order relative to src1 in the rcc
    // command line below is what matters for the bug).
    FILE *f2 = fopen(src2, "w");
    if (!f2) { printf("FAIL: cannot write %s\n", src2); return 1; }
    fputs(
        "int g_ctor_mask;\n"
        "static void __attribute__((constructor)) ctor_b1(void) { g_ctor_mask |= 2; }\n"
        "static void __attribute__((constructor)) ctor_b2(void) { g_ctor_mask |= 4; }\n",
        f2);
    fclose(f2);

    FILE *fm = fopen(srcmain, "w");
    if (!fm) { printf("FAIL: cannot write %s\n", srcmain); return 1; }
    fputs(
        "#include <stdio.h>\n"
        "extern int g_ctor_mask;\n"
        "int main(void) {\n"
        "    if (g_ctor_mask != 7) {\n"
        "        printf(\"FAIL: g_ctor_mask=%d, expected 7\\n\", g_ctor_mask);\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}\n",
        fm);
    fclose(fm);

    // Link order matters: src1 (1 ctor) first, src2 (2 ctors) second --
    // the buggy alignment used the SECOND file's own section byte length
    // as the (bogus) alignment applied when appending it after the first.
    snprintf(buf, sizeof(buf), "%s -o %s %s %s %s " NULL_REDIRECT,
             rcc, exe, src1, src2, srcmain);
    int rc = system(buf);
    remove(src1);
    remove(src2);
    remove(srcmain);
    if (rc != 0) {
        printf("FAIL: link failed (rc=%d)\n", rc);
        return 1;
    }

    rc = system(exe);
    remove(exe);
    // A regression crashes here (SIGSEGV, rc encodes the signal) before
    // ever reaching main()'s own mask check.
    if (rc != 0) {
        printf("FAIL: run failed (rc=%d) -- constructor crashed before main()\n", rc);
        return 1;
    }

    printf("OK constructors from two linked objects run without a spliced null entry\n");
    return 0;
}

#else // _WIN32 || __APPLE__

// Windows (PE .CRT$XCU) and macOS (Mach-O __mod_init_func) use rcc's
// COFF/Mach-O writers, not the native ELF linker this bug lives in;
// skip cleanly rather than assume ELF-specific section layout.
int main(void) { return 0; }

#endif
