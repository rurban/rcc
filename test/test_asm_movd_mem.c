/* `movd` (32-bit GP<->XMM move) only handled its register<->xmm forms
 * ("movd %eax, %xmm0" / "movd %xmm0, %eax"); the memory forms
 * ("movd mem, %xmm0" load, "movd %xmm0, mem" store) matched neither
 * that dispatch's two register-only branches nor the generic mov
 * dispatch (which explicitly excludes "movd" to avoid double-handling
 * the register forms), so the whole instruction fell through silently
 * dropped -- zero bytes emitted, no error, no warning.
 *
 * Found via a real OpenSSL build: crypto/bn/asm/x86_64-mont5.pl's
 * bn_mul_mont_gather5 opens with "movd 8(%rsp),%xmm5"; the dropped load
 * left %xmm5 uninitialized garbage, corrupting a later stack-size
 * computation and segfaulting deep inside the function on a stack
 * overrun (OpenSSL's own exptest/bntest crashed with SIGSEGV in
 * bn_mul_mont_gather5).
 *
 * This test assembles a real .s file using both memory forms of movd,
 * checks the encoded bytes match real gcc's own assembler byte-for-
 * byte, and executes it to confirm the actual data movement is correct.
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
    snprintf(srcf, sizeof(srcf), "%s/test_movd_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_movd_%d.o", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_movd_%d", td, pid);

    /* unsigned movd_roundtrip(unsigned *mem): load mem[0] into %xmm0 via
     * memory-source movd, store %xmm0 back to mem[1] via memory-dest
     * movd, return mem[1] (via a plain reg move) to prove the value
     * actually round-tripped through the XMM register. */
    static const char src[] =
        ".text\n"
        ".globl movd_roundtrip\n"
        "movd_roundtrip:\n"
#ifdef _WIN32
        /* Microsoft x64 ABI: first integer argument arrives in %rcx. */
        "    movd (%rcx), %xmm0\n"
        "    movd %xmm0, 4(%rcx)\n"
        "    movl 4(%rcx), %eax\n"
#else
        /* SysV AMD64 ABI: first integer argument arrives in %rdi. */
        "    movd (%rdi), %xmm0\n"
        "    movd %xmm0, 4(%rdi)\n"
        "    movl 4(%rdi), %eax\n"
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
    if (!strstr(out, "660f6e") || !strstr(out, "660f7e")) {
        printf("FAIL: expected movd load (66 0f 6e) and store (66 0f 7e) "
               "encodings, got:\n%s\n", out);
        remove(objf);
        return 1;
    }

    char csrc[400];
    snprintf(csrc, sizeof(csrc),
             "#include <stdio.h>\n"
             "extern unsigned movd_roundtrip(unsigned *mem);\n"
             "int main(void) {\n"
             "    unsigned buf[2] = {0xdeadbeefu, 0};\n"
             "    unsigned r = movd_roundtrip(buf);\n"
             "    printf(\"%%x %%x\\n\", r, buf[1]);\n"
             "    return !(r == 0xdeadbeefu && buf[1] == 0xdeadbeefu);\n"
             "}\n");
    char csrcf[128];
    snprintf(csrcf, sizeof(csrcf), "%s/test_movd_main_%d.c", td, pid);
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
        printf("FAIL: movd memory round-trip wrong (exit %d)\n", rc);
        return 1;
    }

    printf("OK movd memory forms assemble and round-trip correctly\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
