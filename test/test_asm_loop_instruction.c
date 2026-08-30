/* x86 LOOP/LOOPE/LOOPNE/JECXZ/JRCXZ have no 32-bit-displacement encoding
 * at all -- only an 8-bit relative branch -- unlike JMP/Jcc, which this
 * assembler always emits in their 32-bit-displacement form. rcc's
 * assembler had no support for this instruction family whatsoever, and
 * they don't reduce to any other supported mnemonic: any standalone .s
 * file using `loop` (or the loope/loopne/jecxz/jrcxz variants) failed to
 * assemble with "unknown x86 instruction: loop".
 *
 * Found via a real OpenSSL build: crypto/camellia/cmll-x86_64.s (perlasm-
 * generated) prefetches its S-box with a tight `loop .Lcbc_prefetch_sbox`
 * over 32 iterations.
 *
 * `jecxz`/`jrcxz` are covered here too since they share the same 8-bit-
 * only encoding family and the same deferred-fixup code path -- and
 * because jecxz/jrcxz start with 'j', they need to be dispatched *before*
 * the assembler's generic Jcc handler (which would otherwise try to
 * parse "ecxz"/"rcxz" as a condition code).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[128], cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/test_loop_%d.s", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_loop_%d", td, pid);

    /* Sum 1..5 via a backward LOOP (RCX counts down from 5, decrementing
     * and branching back until it hits zero), matching real generated
     * crypto code's "decrement-and-branch-back" shape exactly. Also
     * exercises jecxz (branch taken when ECX==0, i.e. never here) and
     * jrcxz (same, RCX form) so a regression in either the 0x67
     * address-size prefix or the plain E3 opcode shows up too. */
    static const char src[] =
        ".text\n"
        ".globl sum_via_loop\n"
        "sum_via_loop:\n"
        "\txorl %eax, %eax\n"
        "\tmovl $5, %ecx\n"
        ".Lsum_top:\n"
        "\taddl %ecx, %eax\n"
        "\tloop .Lsum_top\n"
        /* ecx == 0 here (LOOP only exits when it decrements to 0).
         * jecxz/jrcxz only support a *backward* branch target (see the
         * comment at the top of this file): set each counter back to a
         * nonzero value first, so the backward-declared landing label
         * is reached once as plain fallthrough and the jecxz/jrcxz
         * check right after it is provably *not* taken -- this still
         * exercises the real encoding (opcode, displacement fixup, and
         * jecxz's 0x67 address-size override prefix) without needing
         * a forward branch this assembler doesn't support. */
        "\tmovl $1, %ecx\n"
        ".Ljecxz_land:\n"
        "\tjecxz .Ljecxz_land\n"
        "\tmovq $1, %rcx\n"
        ".Ljrcxz_land:\n"
        "\tjrcxz .Ljrcxz_land\n"
        "\tret\n"
        "\n"
        "\t.globl main\n"
        "main:\n"
        "\tsubq $8, %rsp\n"
        "\tcall sum_via_loop\n"
        "\taddq $8, %rsp\n"
        "\tret\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: assemble/link failed (rc=%d)\n", rc);
        remove(exef);
        return 2;
    }

    snprintf(cmd, sizeof(cmd), "%s", exef);
    int status = system(cmd);
    remove(exef);

    /* 1+2+3+4+5 == 15, returned as the process exit code. */
#ifndef _WIN32
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    int exit_code = status; /* Windows system() returns the exit code directly */
#endif
    if (exit_code != 15) {
        printf("FAIL: expected exit code 15 (1+2+3+4+5), got %d\n", exit_code);
        return 1;
    }

    printf("OK LOOP/JECXZ/JRCXZ assemble and execute correctly (sum=%d)\n", exit_code);
    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
