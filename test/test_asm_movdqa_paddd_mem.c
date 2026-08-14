/* MOVDQA/MOVDQU/MOVD/MOVQ (xmm forms) and the packed-integer arithmetic
 * family's (PADDD/PSUBD/PADDQ/PSUBQ/PADDW/PSUBW/PADDB/PSUBB/PAND/POR/
 * PCMPEQD/PCMPGTD) memory-operand form were both silently mis-encoded
 * in the raw-assembly-text dispatch, in three related ways:
 *
 * 1. MOVDQA/MOVDQU/MOVD/MOVQ had either no dispatch entry at all, or
 *    (movaps) an entry that was dead code: asm.c's generic "mov" prefix
 *    dispatch (`!strncmp(mnem, "mov", 3)`) only excluded movsd/movss,
 *    so any other "mov*" mnemonic -- including these -- fell into the
 *    GP-register path first. Its is_reg()/R() macros treat any
 *    "%something" operand as a GP register: parse_x86_reg64() can't
 *    parse "%xmmN" and silently returns X86_NOREG (-1), which aliases
 *    physical register 7 (RDI) once masked into a ModRM field --
 *    "movdqa mem,%xmm0" became "mov mem,%rdi" with no error at all.
 *
 * 2. The packed-integer family's dispatch always called
 *    parse_x86_xmm(ops[0]) unconditionally, even for a real memory
 *    operand: parse_x86_xmm() doesn't recognize a non-"%xmmN" string
 *    and silently falls back to its X86_XMM0 default, so
 *    "paddd mem,%xmmN" silently became "paddd %xmm0,%xmmN" (dropping
 *    the real addend and adding XMM0 to itself instead).
 *
 * 3. x86_movd_xmm_r() (the xmm->GP32 store direction, 66 0F 7E /r) had
 *    its ModRM reg/rm fields backwards -- dead code before this
 *    session (only the load direction, x86_movd_r_xmm(), was ever
 *    called by codegen.c), newly exposed once "movd" got wired into
 *    the dispatch: "movd %xmm3,%eax" encoded as garbage
 *    "movd %xmm0,%ebx" instead.
 *
 * All three were entirely silent (no error, no warning) -- confirmed
 * via test_libsodium's salsa20_xmm6-asm.S, which crashed at runtime
 * from (1), then produced wrong keystream output from (2)+(3) once (1)
 * was fixed; with all three fixed, the real file's own compiled object
 * matches real GNU `as`'s own output instruction-for-instruction
 * (verified via capstone) and produces byte-identical Salsa20 keystream
 * output to a from-spec portable reference implementation.
 *
 * Verified here via the compile-and-check-bytes approach from
 * test_asm_aesni_sse2.c / test_asm_sse_shift_shuffle.c rather than a
 * functional test through inline-asm operand constraints: rcc's
 * inline-asm frontend has its own separate, pre-existing, unrelated
 * bug mangling literal "%%xmmN"/GP register names inside a C-level
 * __asm__ template (reproduces even for long-working instructions like
 * ADDPS, unrelated to this fix) -- out of scope here, see TODO.md.
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
    snprintf(srcf, sizeof(srcf), "%s/test_movdqa_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_movdqa_%s_%d.o", td, tag, pid);

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

    ok &= compile_and_check_bytes(rcc, td, pid, "movdqa_rm",
        ".code64\n.text\n.globl f\nf:\nmovdqa (%rdi),%xmm0\nret\n", "660f6f07c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movdqa_mr",
        ".code64\n.text\n.globl f\nf:\nmovdqa %xmm1,(%rsi)\nret\n", "660f7f0ec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movdqa_rr",
        ".code64\n.text\n.globl f\nf:\nmovdqa %xmm0,%xmm1\nret\n", "660f6fc8c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movdqu_rm",
        ".code64\n.text\n.globl f\nf:\nmovdqu (%rdi),%xmm2\nret\n", "f30f6f17c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movdqu_mr",
        ".code64\n.text\n.globl f\nf:\nmovdqu %xmm2,(%rdx)\nret\n", "f30f7f12c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movq_xmm_rm",
        ".code64\n.text\n.globl f\nf:\nmovq (%rdi),%xmm3\nret\n", "f30f7e1fc3");
    /* movd xmm,r32 store direction (the reg/rm-swap bug) */
    ok &= compile_and_check_bytes(rcc, td, pid, "movd_xmm_r32",
        ".code64\n.text\n.globl f\nf:\nmovd %xmm3,%eax\nret\n", "660f7ed8c3");
    /* "movd" spelled with a 64-bit register: GAS accepts this as
     * movq-with-REX.W (byte-identical to a real "movq" instruction). */
    ok &= compile_and_check_bytes(rcc, td, pid, "movd_xmm_r64",
        ".code64\n.text\n.globl f\nf:\nmovd %xmm3,%rbx\nret\n", "66480f7edbc3");
    /* paddd's memory-operand form -- the parse_x86_xmm() silent-default bug */
    ok &= compile_and_check_bytes(rcc, td, pid, "paddd_mem",
        ".code64\n.text\n.globl f\nf:\npaddd (%r8),%xmm5\nret\n", "66410ffe28c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psubd_mem",
        ".code64\n.text\n.globl f\nf:\npsubd (%rdi),%xmm0\nret\n", "660ffa07c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pand_mem",
        ".code64\n.text\n.globl f\nf:\npand (%rdi),%xmm0\nret\n", "660fdb07c3");

    if (!ok) return 1;
    printf("OK MOVDQA/MOVDQU/MOVD/MOVQ and packed-integer memory-operand "
           "forms all assemble to the correct bytes\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
