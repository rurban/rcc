/* rcc's assembler had zero support for the AES-NI instruction set
 * (AESENC/AESENCLAST/AESDEC/AESDECLAST/AESIMC/AESKEYGENASSIST), several
 * SSE2/SSSE3 instructions whose encoders already existed in x86_enc.c
 * (used internally by codegen.c for `vector_size` types) but were never
 * wired into the raw-assembly-text mnemonic dispatch at all (SHUFPS,
 * PSHUFB, and the whole packed-integer family PADDD/PSUBD/PADDQ/PSUBQ/
 * PADDW/PSUBW/PADDB/PSUBB/PAND/POR/PCMPEQD/PCMPGTD), and several more
 * with no encoder at all (PSHUFD, PSLLDQ/PSRLDQ/PSLLQ/PSRLQ "group 14"
 * shift-by-immediate, PINSRW). Any hand-written `.S` file using any of
 * these failed outright: "error: unknown x86 instruction: aesenc" (etc).
 *
 * Found via LibreSSL's own OpenSSL-derived hand-optimized crypto
 * assembly (`crypto/aes/aesni-*-x86_64.S`, `crypto/modes/ghash-*.S`,
 * `crypto/rc4/rc4-*.S`) failing to assemble at all.
 *
 * Fixed by adding the missing encoders (src/x86_enc.c/.h) and wiring
 * every one of the above into src/asm.c's `encode_x86()` mnemonic
 * dispatch. Every encoding below is verified byte-for-byte identical to
 * real GNU `as`'s own output for the same source (confirmed manually;
 * this test hardcodes those confirmed-correct bytes since GNU `as`
 * itself may not be installed in every CI environment).
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
 * `.text`'s objdump -s output -- mirrors
 * test_asm_gas_directives_batch1.c's compile_and_check_bytes(). Each
 * caller below keeps its whole instruction sequence under 16 bytes
 * (one 16-byte objdump row) so no search string can straddle a row's
 * own offset-prefix column. */
static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *want_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_aesni_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_aesni_%s_%d.o", td, tag, pid);

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

    /* AES-NI family (%xmm1,%xmm2 -- both < 4, no REX needed, keeping
     * bytes exactly GNU-as-comparable). */
    ok &= compile_and_check_bytes(rcc, td, pid, "aesenc",
        ".code64\n.text\n.globl f\nf:\naesenc %xmm1,%xmm2\nret\n", "660f38dcd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aesenclast",
        ".code64\n.text\n.globl f\nf:\naesenclast %xmm1,%xmm2\nret\n", "660f38ddd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aesdec",
        ".code64\n.text\n.globl f\nf:\naesdec %xmm1,%xmm2\nret\n", "660f38ded1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aesdeclast",
        ".code64\n.text\n.globl f\nf:\naesdeclast %xmm1,%xmm2\nret\n", "660f38dfd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aesimc",
        ".code64\n.text\n.globl f\nf:\naesimc %xmm1,%xmm2\nret\n", "660f38dbd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "aeskeygenassist",
        ".code64\n.text\n.globl f\nf:\naeskeygenassist $16,%xmm1,%xmm2\nret\n",
        "660f3adfd110c3");

    /* PSHUFD and the "group 14" shift-by-immediate family. */
    ok &= compile_and_check_bytes(rcc, td, pid, "pshufd",
        ".code64\n.text\n.globl f\nf:\npshufd $85,%xmm1,%xmm2\nret\n", "660f70d155c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pslldq",
        ".code64\n.text\n.globl f\nf:\npslldq $4,%xmm2\nret\n", "660f73fa04c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psrldq",
        ".code64\n.text\n.globl f\nf:\npsrldq $8,%xmm2\nret\n", "660f73da08c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psllq",
        ".code64\n.text\n.globl f\nf:\npsllq $1,%xmm2\nret\n", "660f73f201c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psrlq",
        ".code64\n.text\n.globl f\nf:\npsrlq $63,%xmm2\nret\n", "660f73d23fc3");

    /* PSHUFB and the packed-integer family -- encoders already existed
     * (used internally by codegen.c for vector_size types), only the
     * raw-assembly dispatch was missing. */
    ok &= compile_and_check_bytes(rcc, td, pid, "pshufb",
        ".code64\n.text\n.globl f\nf:\npshufb %xmm1,%xmm2\nret\n", "660f3800d1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "paddd",
        ".code64\n.text\n.globl f\nf:\npaddd %xmm1,%xmm2\nret\n", "660ffed1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psubd",
        ".code64\n.text\n.globl f\nf:\npsubd %xmm1,%xmm2\nret\n", "660ffad1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "paddq",
        ".code64\n.text\n.globl f\nf:\npaddq %xmm1,%xmm2\nret\n", "660fd4d1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psubq",
        ".code64\n.text\n.globl f\nf:\npsubq %xmm1,%xmm2\nret\n", "660ffbd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "paddw",
        ".code64\n.text\n.globl f\nf:\npaddw %xmm1,%xmm2\nret\n", "660ffdd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psubw",
        ".code64\n.text\n.globl f\nf:\npsubw %xmm1,%xmm2\nret\n", "660ff9d1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "paddb",
        ".code64\n.text\n.globl f\nf:\npaddb %xmm1,%xmm2\nret\n", "660ffcd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "psubb",
        ".code64\n.text\n.globl f\nf:\npsubb %xmm1,%xmm2\nret\n", "660ff8d1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pand",
        ".code64\n.text\n.globl f\nf:\npand %xmm1,%xmm2\nret\n", "660fdbd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "por",
        ".code64\n.text\n.globl f\nf:\npor %xmm1,%xmm2\nret\n", "660febd1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pcmpeqd",
        ".code64\n.text\n.globl f\nf:\npcmpeqd %xmm1,%xmm2\nret\n", "660f76d1c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "pcmpgtd",
        ".code64\n.text\n.globl f\nf:\npcmpgtd %xmm1,%xmm2\nret\n", "660f66d1c3");

    /* PINSRW (memory-operand form, base register %rbx < 4 to keep the
     * encoding REX-free and exactly GNU-as-comparable -- a base/index
     * register in the 4-7 range triggers a pre-existing, purely
     * cosmetic quirk in maybe_rex() that emits a harmless,
     * semantically-inert extra REX 0x40 byte, correct but not
     * byte-identical to GNU as; unrelated to this fix). */
    ok &= compile_and_check_bytes(rcc, td, pid, "pinsrw",
        ".code64\n.text\n.globl f\nf:\npinsrw $3,(%rbx,%rax,4),%xmm2\nret\n",
        "660fc4148303c3");

    if (!ok) return 1;
    printf("OK AES-NI, packed-SSE2-integer, PSHUFD/group-14-shift, and "
           "PINSRW all assemble to the correct bytes\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
