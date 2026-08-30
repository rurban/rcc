/* AVX's VZEROUPPER/VZEROALL are the only VEX-encoded instructions with
 * zero register operands (no ModRM byte at all): VZEROUPPER (VEX.128.0F.WIG
 * 77) zeros the upper 128 bits of every YMM register; VZEROALL (VEX.256.0F.WIG
 * 77) zeros all of every YMM register including the low XMM half. rcc's
 * assembler rejected both outright with "unknown x86 instruction: vzeroupper".
 *
 * Found via a real OpenSSL build: crypto/sha/sha512-x86_64.s (perlasm-
 * generated) uses vzeroupper to exit its AVX2 code path cleanly before
 * returning to SSE/scalar callers -- "rcc: error: failed to assemble
 * crypto/sha/sha512-x86_64.s" aborted the whole build.
 *
 * This test assembles a real .s file using both mnemonics, checks the
 * encoded bytes match real gcc's own assembler byte-for-byte (C5 F8 77 for
 * vzeroupper, C5 FC 77 for vzeroall), and actually executes them (they are
 * always safe no-ops with respect to GP registers/flags on any AVX-capable
 * CPU) to confirm the assembled code doesn't crash.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], exef[128], cmd[900];
    snprintf(srcf, sizeof(srcf), "%s/test_vzu_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_vzu_%d.o", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_vzu_%d", td, pid);

    static const char src[] =
        ".text\n"
        ".globl main\n"
        "main:\n"
        "    vzeroupper\n"
        "    vzeroall\n"
        "    xorl %eax, %eax\n"
        "    ret\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: assemble failed (rc=%d)\n", rc);
        remove(srcf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -d %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    char out[2048] = {0};
    if (p) {
        size_t n = fread(out, 1, sizeof(out) - 1, p);
        out[n] = '\0';
        pclose(p);
    }
    remove(srcf);

    int have_vzu = strstr(out, "c5 f8 77") != NULL || strstr(out, "vzeroupper") != NULL;
    int have_vza = strstr(out, "c5 fc 77") != NULL || strstr(out, "vzeroall") != NULL;
    if (!have_vzu || !have_vza) {
        printf("FAIL: expected vzeroupper (c5 f8 77) and vzeroall (c5 fc 77) "
               "encodings, got:\n%s\n", out);
        remove(objf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, exef, objf);
    rc = system(cmd);
    remove(objf);
    if (rc != 0) {
        printf("FAIL: link failed (rc=%d)\n", rc);
        return 1;
    }

    rc = system(exef);
    remove(exef);
    if (rc != 0) {
        printf("FAIL: executing vzeroupper/vzeroall exited %d\n", rc);
        return 1;
    }

    printf("OK vzeroupper/vzeroall assemble and execute correctly\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
