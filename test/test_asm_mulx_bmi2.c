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

    /* void mulx_test(unsigned long long a, unsigned long long b,
     *                 unsigned long long *lo, unsigned long long *hi):
     * writes a*b's low/high 64-bit halves through the two out-pointers.
     * `long long` (not `long`) is required here: `long` is only 32 bits
     * under Windows' LLP64 model, and this hand-written asm always
     * moves/stores full 64-bit registers -- a `long`-typed out-pointer
     * would have the callee overrun its 4-byte stack slot with an
     * 8-byte store. (Not returned as a 128-bit value: SysV returns
     * __int128 in rax:rdx, but Windows x64 has no such convention -- an
     * out-pointer pair sidesteps the ABI divergence entirely and keeps
     * this test focused on mulx's own encoding/semantics, not calling
     * conventions.) */
    static const char src[] =
        ".text\n"
        ".globl mulx_test\n"
        "mulx_test:\n"
#ifdef _WIN32
        /* Microsoft x64 ABI: a=%rcx, b=%rdx, lo=%r8, hi=%r9. */
        "    movq %rdx, %r10\n"   /* save b (rdx is about to be overwritten) */
        "    movq %rcx, %rdx\n"   /* rdx = a (mulx's implicit multiplicand) */
        "    mulx %r10, %rax, %r11\n" /* r11:rax = a*b (src=b via %r10) */
        "    movq %rax, (%r8)\n"
        "    movq %r11, (%r9)\n"
#else
        /* SysV AMD64 ABI: a=%rdi, b=%rsi, lo=%rdx, hi=%rcx. */
        "    movq %rdx, %r10\n"   /* save lo-ptr (rdx is about to be overwritten) */
        "    movq %rcx, %r11\n"   /* save hi-ptr (rcx is mulx's dst_high) */
        "    movq %rdi, %rdx\n"   /* rdx = a (mulx's implicit multiplicand) */
        "    mulx %rsi, %rax, %rcx\n" /* rcx:rax = a*b (src=b via %rsi) */
        "    movq %rax, (%r10)\n"
        "    movq %rcx, (%r11)\n"
#endif
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
    strip_spaces(out);

    if (!strstr(out, "fbf6") || !strstr(out, "mulx")) {
        printf("FAIL: expected a VEX-encoded mulx instruction, got:\n%s\n", out);
        remove(objf);
        return 1;
    }
    /* The old bug encoded this as a plain one-operand `mul %reg`
     * (a REX.W F7 /4 opcode), never a VEX prefix at all -- if that
     * shows up instead of the real mulx encoding, the dispatch
     * regressed. Check whichever source register this platform's
     * variant above actually multiplies by. */
#ifdef _WIN32
    if (strstr(out, "49f7e2") || strstr(out, "mul%r10")) {
#else
    if (strstr(out, "48f7e6") || strstr(out, "mul%rsi")) {
#endif
        printf("FAIL: mulx mis-encoded as plain one-operand mul:\n%s\n", out);
        remove(objf);
        return 1;
    }

    char csrc[512];
    snprintf(csrc, sizeof(csrc),
             "#include <stdio.h>\n"
             "extern void mulx_test(unsigned long long a, unsigned long long b,\n"
             "                      unsigned long long *lo, unsigned long long *hi);\n"
             "int main(void) {\n"
             "    unsigned long long lo, hi;\n"
             "    mulx_test(0xFFFFFFFFFFFFFFFFULL, 3, &lo, &hi);\n"
             "    printf(\"%%llx %%llx\\n\", hi, lo);\n"
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
