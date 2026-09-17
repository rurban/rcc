/* REX prefix was emitted more often than the x86-64 encoding rules
 * require, in two independent ways:
 *
 * 1. maybe_rex()'s own "should I emit a REX byte at all" gate compared
 *    each of its R/X/B register-field arguments against X86_RSP (4)
 *    instead of X86_RDI (7): any GPR operand in the RSP/RBP/RSI/RDI
 *    range (4-7) forced a REX byte even though those four encode in
 *    3 bits with no REX at all outside byte-sized ops (only R8-R15,
 *    values 8-15, ever need REX.R/X/B). "mov %esi,%edi" emitted
 *    "40 89 f7" instead of GAS's "89 f7".
 *
 * 2. Ten encoders (x86_lea, ADD/SUB/AND/OR/XOR/CMP's memory-operand
 *    forms, IMUL, CMOVcc, XCHG, and the shared BSF/BSR/POPCNT/LZCNT/
 *    TZCNT/BT (bit-test)/XADD/CMPXCHG reg-reg helper) called rex()
 *    directly and
 *    unconditionally instead of through maybe_rex()/a needrex guard,
 *    so they emitted a REX byte on *every* call regardless of whether
 *    any operand needed one at all -- e.g. "imul %esi,%edi" (both
 *    operands < R8) still got a REX 0x40. A parallel bug hit ~110
 *    SSE4.1/4.2/SSSE3/AES-NI/SHA-NI/PCLMULQDQ two-register-operand
 *    encoders and several memory-operand ones (CRC32, x87 loads,
 *    PINSRW/PEXTRD's memory forms): they called maybe_rex() correctly,
 *    but passed a *raw*, unguarded register number for R/B instead of
 *    pre-clamping it, so bug 1's gate threshold made XMM4-XMM7 (and,
 *    for the memory forms, a base/index register in RSP-RDI) look
 *    exactly like it needed a REX byte too.
 *
 * Both classes produced semantically-correct-but-non-minimal encodings
 * (a redundant REX 0x40 is a harmless no-op byte) -- not a correctness
 * bug in itself, EXCEPT that fixing bug 1's gate also exposed a real,
 * previously-masked, separate bug: suffix_size() defaulted to 8
 * (64-bit) for any CMOVcc mnemonic whose condition-code letter wasn't
 * itself q/l/w/b (e.g. "cmove", ending in 'e'), forcing REX.W=1 and
 * silently reinterpreting "cmove %esi,%edi" as a 64-bit cmove on
 * %rsi/%rdi -- previously invisible because every CMOVcc already got a
 * REX byte for the wrong reason (bug 1's own gate), so the REX.W bit's
 * own separate correctness impact was masked by that byte simply
 * always being present anyway.
 *
 * Fixed by tightening maybe_rex()'s gate to "> X86_RDI", converting the
 * ten unconditional rex() callers to maybe_rex(), fixing the ~110
 * XMM/memory-operand call sites to pass raw (now-safe) register values,
 * and excluding CMOVcc from suffix_size()'s last-letter heuristic
 * entirely (its trailing letter is always a condition code, never a
 * size suffix). Every encoding below is verified byte-for-byte
 * identical to real GNU `as`'s own output for the same source
 * (confirmed manually; this test hardcodes those confirmed-correct
 * bytes since GNU `as` itself may not be installed in every CI
 * environment).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* Assembles `src` as a standalone .S file and checks that `want_hex`
 * appears as a contiguous, whitespace-collapsed hex substring in
 * `.text`'s objdump -s output -- mirrors test_asm_aesni_sse2.c's
 * compile_and_check_bytes(). Each caller below keeps its whole
 * instruction sequence under 16 bytes (one 16-byte objdump row) so no
 * search string can straddle a row's own offset-prefix column. */
static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *want_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_rexmin_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_rexmin_%s_%d.o", td, tag, pid);

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

    /* Plain GPR ops on RSP/RBP/RSI/RDI-range registers: maybe_rex()'s
     * gate threshold bug (class 1). */
    ok &= compile_and_check_bytes(rcc, td, pid, "mov_esi_edi",
        ".code64\n.text\n.globl f\nf:\nmov %esi,%edi\nret\n", "89f7c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "add_ebp_esp",
        ".code64\n.text\n.globl f\nf:\nadd %ebp,%esp\nret\n", "01ecc3");

    /* Unconditional-rex() callers (class 2, scalar): LEA, memory-dest
     * ALU, IMUL, CMOVcc, BSF, XCHG (reg-reg and reg-mem). */
    ok &= compile_and_check_bytes(rcc, td, pid, "lea_rsi_base",
        ".code64\n.text\n.globl f\nf:\nlea (%rsi,%rax,4),%edi\nret\n", "8d3c86c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "add_mem_rsi",
        ".code64\n.text\n.globl f\nf:\naddl %eax,(%rsi)\nret\n", "0106c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmp_mem_rsi",
        ".code64\n.text\n.globl f\nf:\ncmpl %eax,(%rsi)\nret\n", "3906c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "imul_esi_edi",
        ".code64\n.text\n.globl f\nf:\nimul %esi,%edi\nret\n", "0faffec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "bsf_esi_edi",
        ".code64\n.text\n.globl f\nf:\nbsf %esi,%edi\nret\n", "0fbcfec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "xchg_esi_edi",
        ".code64\n.text\n.globl f\nf:\nxchg %esi,%edi\nret\n", "87f7c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "xchg_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nxchg %eax,(%rsi)\nret\n", "8706c3");

    /* CMOVcc: exercises both the class-2 unconditional-rex() fix AND
     * the suffix_size() cmov-condition-letter fix it exposed -- a
     * condition ending in a real size letter ("cmovl"=Less, "cmovb"
     * =Below) vs one that isn't ("cmovne", "cmovg", "cmova"). */
    ok &= compile_and_check_bytes(rcc, td, pid, "cmove_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmove %esi,%edi\nret\n", "0f44fec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmovne_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmovne %esi,%edi\nret\n", "0f45fec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmovg_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmovg %esi,%edi\nret\n", "0f4ffec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmovl_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmovl %esi,%edi\nret\n", "0f4cfec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmovb_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmovb %esi,%edi\nret\n", "0f42fec3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmova_esi_edi",
        ".code64\n.text\n.globl f\nf:\ncmova %esi,%edi\nret\n", "0f47fec3");

    /* Unguarded-maybe_rex() callers (class 2, XMM4-XMM7): the ~110-site
     * SSE4.1/4.2/SSSE3 bug -- neither operand is R8-R15, so no REX at
     * all is needed for any of these, unlike the pre-existing xmm1/
     * xmm2 (< R8) cases test_asm_aesni_sse2.c already covered. */
    ok &= compile_and_check_bytes(rcc, td, pid, "pxor_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npxor %xmm5,%xmm4\nret\n", "660fefe5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "xorps_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\nxorps %xmm5,%xmm4\nret\n", "0f57e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aesenc_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\naesenc %xmm5,%xmm4\nret\n", "660f38dce5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pcmpeqq_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npcmpeqq %xmm5,%xmm4\nret\n", "660f3829e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pcmpgtq_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npcmpgtq %xmm5,%xmm4\nret\n", "660f3837e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "ptest_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\nptest %xmm5,%xmm4\nret\n", "660f3817e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pmulld_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npmulld %xmm5,%xmm4\nret\n", "660f3840e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "phaddw_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\nphaddw %xmm5,%xmm4\nret\n", "660f3801e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pabsd_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npabsd %xmm5,%xmm4\nret\n", "660f381ee5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pblendvb_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\npblendvb %xmm5,%xmm4\nret\n", "660f3810e5c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "roundps_imm_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\nroundps $1,%xmm5,%xmm4\nret\n", "660f3a08e501c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "blendps_imm_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\nblendps $2,%xmm5,%xmm4\nret\n", "660f3a0ce502c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmpps_imm_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\ncmpps $4,%xmm5,%xmm4\nret\n", "0fc2e504c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "cmpsd_imm_xmm4_xmm5",
        ".code64\n.text\n.globl f\nf:\ncmpsd $0,%xmm5,%xmm4\nret\n", "f20fc2e500c3");

    /* Unguarded-maybe_rex() callers (class 2, memory-operand m.base):
     * CRC32, MOVNTDQA/MOVNTPS/MOVNTI, LDDQU, x87 FLDL all took the
     * base/index register raw instead of pre-clamping it, same bug as
     * the reg-reg XMM4-7 cases above but for RSP-RDI-range base regs. */
    ok &= compile_and_check_bytes(rcc, td, pid, "crc32b_mem_rsi",
        ".code64\n.text\n.globl f\nf:\ncrc32b (%rsi),%eax\nret\n", "f20f38f006c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movntdqa_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nmovntdqa (%rsi),%xmm4\nret\n", "660f382a26c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movntps_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nmovntps %xmm4,(%rsi)\nret\n", "0f2b26c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "movnti_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nmovnti %eax,(%rsi)\nret\n", "0fc306c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "lddqu_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nlddqu (%rsi),%xmm4\nret\n", "f20ff026c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "fldl_mem_rsi",
        ".code64\n.text\n.globl f\nf:\nfldl (%rsi)\nret\n", "dd06c3");

    if (!ok) return 1;
    printf("OK REX bytes are minimal: GPR/XMM4-7/memory-operand ops on "
           "RSP-RDI-range registers, plus the CMOVcc size-inference fix "
           "it exposed, all assemble to GNU as's own bytes\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
