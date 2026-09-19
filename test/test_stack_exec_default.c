/* PT_GNU_STACK was unconditionally emitted with PF_X (executable stack)
 * for every non-`-shared` link, regardless of whether any linked-in
 * object actually required it. Nested-function trampolines (see
 * codegen.c's ND_LVAR function-value branch) are the only thing that
 * legitimately needs an executable stack; ordinary code doesn't.
 *
 * Fixed by tracking, while loading each ELF object, whether its
 * `.note.GNU-stack` section is missing (pre-marker-era object, treated
 * conservatively like GNU ld does) or carries SHF_EXECINSTR, and only
 * then requesting PF_X on the output PT_GNU_STACK -- matching GNU ld's
 * own algorithm and what `gcc`-linked binaries produce.
 *
 * GH #63. ELF-only: PT_GNU_STACK is an ELF ABI concept with no direct
 * equivalent on Windows/mingw's PE-COFF or macOS's Mach-O.
 */
#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__APPLE__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

// Links `src` into an executable and returns whether its PT_GNU_STACK
// program header requests an executable stack ('E' in readelf -Wl's
// "RWE"/"RW " Flg column). Returns -1 on any compile/readelf failure.
static int link_needs_exec_stack(const char *rcc, const char *td, int pid,
                                 const char *tag, const char *src) {
    char srcf[128], outf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_stackexec_%s_%d.c", td, tag, pid);
    snprintf(outf, sizeof(outf), "%s/test_stackexec_%s_%d.out", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return -1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, outf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile/link failed\n", tag);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "readelf -Wl %s " NULL_REDIRECT, outf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] readelf failed to run\n", tag); remove(outf); return -1; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(outf);

    char *line = strstr(out, "GNU_STACK");
    if (!line) { printf("FAIL: [%s] no GNU_STACK program header\n", tag); return -1; }
    char *flg = strstr(line, "RW");
    if (!flg) { printf("FAIL: [%s] no RW.. flags column after GNU_STACK\n", tag); return -1; }
    return flg[2] == 'E';
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    // Plain code needs no executable stack.
    int r = link_needs_exec_stack(rcc, td, pid, "plain",
        "int add(int a, int b) { return a + b; }\n"
        "int main(void) { return add(1, 2) == 3 ? 0 : 1; }\n");
    if (r != 0) {
        printf("FAIL: plain binary got exec stack (r=%d)\n", r);
        ok = 0;
    }

#if defined(__linux__)
    // A nested function whose address is taken (used as a value, not
    // just called directly) requires a stack-resident trampoline and so
    // legitimately needs an executable stack -- verified only where
    // rcc's own native ELF linker (link_elf.c) actually performs the
    // link. FreeBSD/OpenBSD/NetBSD fall back to the system cc/ld for
    // crt1.o layouts rcc doesn't special-case yet (see NEWS v1.2.2),
    // so whether the resulting stack ends up executable there depends
    // on that system toolchain's own policy, not this fix.
    r = link_needs_exec_stack(rcc, td, pid, "nested",
        "static int call_ptr(int (*fn)(int *), int *p) { return fn(p); }\n"
        "static int outer(void) {\n"
        "    int i = 0;\n"
        "    int inner(int *p) { i = 1; return *p + 1; }\n"
        "    return call_ptr(inner, &i);\n"
        "}\n"
        "int main(void) { return outer() == 2 ? 0 : 1; }\n");
    if (r != 1) {
        printf("FAIL: nested-trampoline binary did not get exec stack (r=%d)\n", r);
        ok = 0;
    }
#endif

    if (ok) printf("OK\n");
    return ok ? 0 : 1;
}
#else
int main(void) {
    return 0;
}
#endif
