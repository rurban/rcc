/* The built-in assembler's GAS ".macro" pre-pass (asm.c) capped the number
 * of distinct macro definitions it would remember per translation unit at
 * 16, hard-coded via a fixed-size array — and silently dropped every
 * definition past that limit with no diagnostic at all: not an error, not
 * even a warning. Every later invocation of a dropped macro then fell
 * through to the plain instruction encoder, which (correctly, given no
 * macro by that name exists as far as it can tell) reported "unknown x86
 * instruction" for the macro's own name and emitted nothing for it.
 *
 * Found via a real Linux kernel build: arch/x86/entry/entry.S's full
 * #include chain (include/linux/objtool.h, arch/x86/include/asm/
 * {unwind_hints,nospec-branch,alternative}.h, ...) registers over 50
 * distinct macros before any of entry.S's own code is reached — nearly
 * one and a half orders of magnitude past the old 16-macro cap.
 * FILL_RETURN_BUFFER (used near the very start of entry.S to stuff the
 * return-stack buffer for a Spectre mitigation) is roughly the 24th macro
 * definition in file order and was silently dropped well before rcc ever
 * saw its actual invocation.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

#define OLD_CAP 16
#define NUM_MACROS (OLD_CAP + 8) /* comfortably past the old fixed limit */

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[160], cmd[8192];
    snprintf(srcf, sizeof(srcf), "%s/test_amc_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_amc_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif

    /* NUM_MACROS trivial, distinct macros defined in a row (mirrors real
     * kernel headers registering many small ".macro" definitions well
     * before any of them actually gets used), each called in turn from
     * main — every one but the last just sets %eax to its own index; the
     * *last* one (well past the old 16-macro cap) sets it to a
     * distinctive value instead, so main's return code alone proves the
     * whole run past the old cap works. Compile straight to a runnable
     * executable and check the exit code rather than inspecting the
     * object file directly: reassembling objdump's hex-dump output across
     * its per-row address-column formatting (needed once the payload
     * exceeds one 16-byte row) is exactly the kind of incidental
     * complexity this test shouldn't depend on. */
    char src[8192];
    size_t off = 0;
    for (int i = 0; i < NUM_MACROS - 1 && off + 128 < sizeof(src); i++)
        off += (size_t)snprintf(src + off, sizeof(src) - off,
                                ".macro MAC%d\n\tmovl $%d, %%eax\n.endm\n", i, i);
    off += (size_t)snprintf(src + off, sizeof(src) - off,
                            ".macro MAC%d\n\tmovl $42, %%eax\n.endm\n", NUM_MACROS - 1);
    /* Mach-O requires a leading underscore on "main" (the runtime startup
     * code looks for "_main" specifically on Darwin, unlike ELF/COFF's
     * bare "main"); the MAC* labels are only ever referenced within this
     * same file and don't need it. */
#ifdef __APPLE__
    off += (size_t)snprintf(src + off, sizeof(src) - off, ".text\n.globl _main\n_main:\n");
#else
    off += (size_t)snprintf(src + off, sizeof(src) - off, ".text\n.globl main\nmain:\n");
#endif
    for (int i = 0; i < NUM_MACROS && off + 32 < sizeof(src); i++)
        off += (size_t)snprintf(src + off, sizeof(src) - off, "\tMAC%d\n", i);
    off += (size_t)snprintf(src + off, sizeof(src) - off, "\tret\n");

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s -nostdinc -D__ASSEMBLY__ " NULL_REDIRECT,
             rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile+link failed (rc=%d)\n", rc);
        remove(exef);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    int status = system(cmd);
    remove(exef);
#ifndef _WIN32
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    int exit_code = status; /* Windows system() returns the exit code directly */
#endif

    /* If MAC(NUM_MACROS-1) — definition #NUM_MACROS, past the old
     * 16-macro cap — had been silently dropped, its invocation would fall
     * through as an unrecognized instruction (a no-op as far as codegen is
     * concerned) and main would return whatever the second-to-last macro
     * (MAC(NUM_MACROS-2), setting %eax to NUM_MACROS-2) left behind
     * instead of 42. */
    if (exit_code != 42) {
        printf("FAIL: expected exit code 42 from macro #%d (past the old "
               "%d-macro cap), got %d\n", NUM_MACROS, OLD_CAP, exit_code);
        return 1;
    }

    printf("OK %d .macro definitions in one translation unit (past the old "
           "%d-macro cap) all register and expand correctly\n", NUM_MACROS, OLD_CAP);
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
