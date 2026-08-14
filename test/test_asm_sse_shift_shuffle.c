/* Inline-asm/`.S` x86 assembler gaps: PSHUFLW, PSHUFHW, and the
 * PSLLD/PSRLD Group-13 immediate-shift form were rejected with "error:
 * unknown x86 instruction". x86_enc.c already had encoders for
 * PSHUFLW/PSHUFHW (used by the compiler-intrinsic path) but they were
 * never wired into asm.c's raw-assembly mnemonic dispatch; PSLLD/PSRLD
 * had only a register-count-shift encoder (x86_pslld_r/x86_psrld_r),
 * not the immediate-shift form real hand-written SIMD assembly (e.g.
 * nettle's) actually uses -- added new Group 13 (66 0F 72 /ext ib)
 * encoders mirroring the existing Group 14 (0F 73) helper backing
 * pslldq/psrldq/psllq/psrlq.
 *
 * Found via test_nettle. Mirrors test_asm_aesni_sse2.c's
 * compile-and-check-bytes approach: every encoding below is verified
 * byte-for-byte identical to real GNU `as`'s own output for the same
 * source (confirmed manually; this test hardcodes those
 * confirmed-correct bytes since GNU `as` itself may not be installed
 * in every CI environment).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *want_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_sseshift_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_sseshift_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: [%s] cannot write %s\n", tag, srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .text %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[4096];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = (char)tolower((unsigned char)*s);
    collapsed[cn] = '\0';

    if (!strstr(collapsed, want_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" in .text, got:\n%s\n",
               tag, want_hex, out);
        return 0;
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    ok &= compile_and_check_bytes(rcc, td, pid, "pshuflw",
        ".code64\n.text\n.globl f\nf:\npshuflw $0x1b,%xmm0,%xmm1\nret\n",
        "f20f70c81bc3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pshufhw",
        ".code64\n.text\n.globl f\nf:\npshufhw $0x1b,%xmm2,%xmm3\nret\n",
        "f30f70da1bc3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pslld",
        ".code64\n.text\n.globl f\nf:\npslld $5,%xmm0\nret\n", "660f72f005c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psrld",
        ".code64\n.text\n.globl f\nf:\npsrld $7,%xmm1\nret\n", "660f72d107c3");
    /* XMM8-15 destination: exercises the REX.B path. */
    ok &= compile_and_check_bytes(rcc, td, pid, "pslld_rex",
        ".code64\n.text\n.globl f\nf:\npslld $3,%xmm9\nret\n", "66410f72f103c3");

    if (!ok) return 1;
    printf("OK PSHUFLW/PSHUFHW and PSLLD/PSRLD Group-13 shift all "
           "assemble to the correct bytes\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
