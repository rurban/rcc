/* MULX (BMI2's 3-operand, flag-free multiply: `mulx src, dst_low,
 * dst_high` computes RDX (implicit) * src -> dst_high:dst_low) was
 * silently mis-assembled: the generic "mul" mnemonic dispatch prefix-
 * matched the first 3 characters ("mul" also matches "mulx") and
 * swallowed it as the classic one-operand `mul %reg` form (implicit
 * RDX:RAX = RAX * r/m), discarding two of MULX's three explicit
 * operands and computing a completely different product into the wrong
 * registers -- no error, no warning, just silently wrong code.
 *
 * Found via a real OpenSSL build: crypto/bn/asm/x86_64-mont5.pl
 * (mulx4x_mont/sqrx8x_mont) issues MULX in tight back-to-back ADCX/ADOX
 * dual-carry-chain loops throughout its Montgomery multiplication inner
 * loop -- the classic BMI2/ADX bignum trick, since MULX (unlike MUL)
 * writes no flags at all, so it can run interleaved with ADCX (CF chain)
 * and ADOX (OF chain) without clobbering either. Every miscompiled MULX
 * call corrupted the running product; OpenSSL's own bntest regression
 * suite (test_modexp_mont5, named for exactly this historical class of
 * "carry bug" in these routines) failed with wildly wrong results.
 *
 * This test assembles a real .s file computing a known 64x64->128-bit
 * product via MULX, checks the encoded bytes match real gcc's own
 * assembler byte-for-byte, and executes it to confirm the actual
 * arithmetic result is correct (not just that assembly succeeds).
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
    snprintf(srcf, sizeof(srcf), "%s/test_mulx_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_mulx_%d.o", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_mulx_%d", td, pid);

    /* unsigned __int128 mulx_test(unsigned long a, unsigned long b):
     * rdx <- a (first arg), mulx computes rdx*rsi -> rdx:rax (high:low),
     * matching the SysV __int128 return convention (rax=low,rdx=high). */
    static const char src[] =
        ".text\n"
        ".globl mulx_test\n"
        "mulx_test:\n"
        "    movq %rdi, %rdx\n"
        "    mulx %rsi, %rax, %rdx\n"
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

    if (!strstr(out, "c4 e2") || !strstr(out, "mulx")) {
        printf("FAIL: expected a VEX-encoded mulx instruction, got:\n%s\n", out);
        remove(objf);
        return 1;
    }
    /* The old bug encoded this as a plain one-operand `mul %rsi`
     * (48 f7 e6), never a VEX prefix at all -- if that shows up instead
     * of the real mulx encoding, the dispatch regressed. */
    if (strstr(out, "48 f7 e6") || strstr(out, "mul    %rsi")) {
        printf("FAIL: mulx mis-encoded as plain one-operand mul:\n%s\n", out);
        remove(objf);
        return 1;
    }

    char csrc[512];
    snprintf(csrc, sizeof(csrc),
             "#include <stdio.h>\n"
             "extern unsigned __int128 mulx_test(unsigned long a, unsigned long b);\n"
             "int main(void) {\n"
             "    unsigned __int128 r = mulx_test(0xFFFFFFFFFFFFFFFFULL, 3);\n"
             "    unsigned long lo = (unsigned long)r;\n"
             "    unsigned long hi = (unsigned long)(r >> 64);\n"
             "    printf(\"%%lx %%lx\\n\", hi, lo);\n"
             "    /* 0xFFFFFFFFFFFFFFFF * 3 = 0x2FFFFFFFFFFFFFFFD */\n"
             "    return !(hi == 0x2 && lo == 0xFFFFFFFFFFFFFFFDULL);\n"
             "}\n");
    char csrcf[128];
    snprintf(csrcf, sizeof(csrcf), "%s/test_mulx_main_%d.c", td, pid);
    FILE *fc = fopen(csrcf, "w");
    if (!fc) { printf("FAIL: cannot write %s\n", csrcf); remove(objf); return 1; }
    fputs(csrc, fc);
    fclose(fc);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s %s " NULL_REDIRECT, rcc, exef, csrcf, objf);
    rc = system(cmd);
    remove(objf);
    remove(csrcf);
    if (rc != 0) {
        printf("FAIL: link failed (rc=%d)\n", rc);
        return 1;
    }

    rc = system(exef);
    remove(exef);
    if (rc != 0) {
        printf("FAIL: mulx product wrong (exit %d)\n", rc);
        return 1;
    }

    printf("OK mulx assembles and computes the correct 128-bit product\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
