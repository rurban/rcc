/* Taking the address of an external (declared-only, defined elsewhere)
 * function under -fPIE/-fPIC must load it through the GOT: the symbol's
 * real address isn't known until runtime (it may live in libc.so, and
 * even for a same-binary symbol the dynamic linker may preempt it), so
 * the compiler cannot bake in a direct RIP-relative offset.
 *
 * Regression: -fPIE/-fpie only set a link-only `opt_pie` flag; codegen's
 * `var_needs_got()` (the switch between a GOT-indirected `mov` and a
 * direct `lea`) checked only `opt_pic`, which plain -fPIE never set.
 * Every external function whose *address* was taken (as opposed to
 * directly called, which already correctly went through the PLT) under
 * -fPIE got a bare `lea reg, sym` with an R_X86_64_PC32 relocation
 * instead of `mov reg, [rip+GOT]` with R_X86_64_GOTPCREL.
 *
 * Found via a real OpenSSH build: sshbuf-io.c takes `write`'s address
 * (stored as a function-pointer field), and openssh's whole tree is
 * compiled with -fPIE; the resulting PC32 relocation against the
 * preemptible glibc symbol `write` made the linker reject the object
 * outright ("relocation R_X86_64_PC32 against symbol `write@@GLIBC_*`
 * can not be used when making a PIE object").
 *
 * This test only exercises the *compile-time* relocation choice (readelf
 * on the .o), not a full PIE link/run, since the test harness's own
 * build may not itself be position-independent. ELF-only: -fPIE/GOT/PLT
 * relocations (and readelf itself) are an ELF ABI concept with no direct
 * equivalent on Windows/mingw's PE-COFF or macOS's Mach-O, so this is
 * skipped on those targets.
 */
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__APPLE__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_pie_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_pie_%d.o", td, pid);

    static const char src[] =
        "#include <unistd.h>\n"
        "typedef ssize_t (*write_fn)(int, const void *, size_t);\n"
        "write_fn get_write(void) { return write; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -fPIE -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 2;
    }

    snprintf(cmd, sizeof(cmd), "readelf -r %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: readelf -r failed\n"); remove(objf); return 3; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    if (!strstr(out, "GOTPCREL") || !strstr(out, "write")) {
        printf("FAIL: expected an R_X86_64_GOTPCREL relocation against `write`, got:\n%s\n", out);
        return 1;
    }
    if (strstr(out, "R_X86_64_PC32") && strstr(strstr(out, "R_X86_64_PC32"), "write")) {
        printf("FAIL: found a direct PC32 relocation against `write` under -fPIE:\n%s\n", out);
        return 1;
    }

    printf("OK taking an external function's address under -fPIE uses the GOT\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
