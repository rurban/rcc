/* pushf/pushfq, popf/popfq and invlpg were entirely unimplemented in
 * encode_x86(): asm.c's mnemonic dispatch fell through to the "unknown x86
 * instruction" warning and silently emitted zero bytes for the whole
 * instruction, no error. That's a correctness bug beyond just "missing
 * warning noise": every later instruction/label/relocation offset in the
 * same section shifted left by however many bytes the dropped instruction
 * should have occupied, desyncing anything computed from that layout
 * (jump targets, objtool's instruction decoding, ...).
 *
 * Found via a real Linux kernel build: arch/x86/kernel/verify_cpu.S and
 * arch/x86/entry/entry.S both save/restore EFLAGS around a call with
 * "pushf"/"popf" (no operands — PUSHF/POPF have no 32-bit form in 64-bit
 * mode, so the plain and "q"-suffixed spellings assemble identically), and
 * arch/x86/mm/tlb.c's local_flush_tlb_one() etc. use "invlpg (%0)" via
 * inline asm to invalidate a single TLB entry.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int check_disasm_contains(const char *rcc, const char *td, int pid,
                                 const char *tag, const char *src,
                                 const char *const *want, int nwant)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_pfi_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_pfi_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -d -j .text %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    for (int i = 0; i < nwant; i++) {
        if (!strstr(out, want[i])) {
            printf("FAIL: [%s] expected \"%s\" in disassembly, got:\n%s\n",
                   tag, want[i], out);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    static const char src_pushf[] =
        "int main(void) { __asm__ volatile(\"pushf\\n\\tpopf\\n\\t\"); return 0; }\n";
    /* objdump's AT&T disassembly names the opcode "pushf"/"popf" regardless
     * of whether the source used the bare or "q"-suffixed spelling — both
     * assemble to the exact same single byte in 64-bit mode. */
    static const char *want_pushf[] = {"pushf", "popf"};
    ok &= check_disasm_contains(rcc, td, pid, "pushf", src_pushf, want_pushf, 2);

    static const char src_pushfq[] =
        "int main(void) { __asm__ volatile(\"pushfq\\n\\tpopfq\\n\\t\"); return 0; }\n";
    ok &= check_disasm_contains(rcc, td, pid, "pushfq", src_pushfq, want_pushf, 2);

    static const char src_invlpg[] =
        "int main(void) { void *p = 0; "
        "__asm__ volatile(\"invlpg (%0)\\n\\t\" : : \"r\"(p) : \"memory\"); return 0; }\n";
    static const char *want_invlpg[] = {"invlpg"};
    ok &= check_disasm_contains(rcc, td, pid, "invlpg", src_invlpg, want_invlpg, 1);

    /* Smoke test: pushf/popf must round-trip flags without corrupting them
     * (a real EFLAGS save/restore, not just "assembles to something"). */
    unsigned long saved, restored;
    __asm__ volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "push %0\n\t"
        "popf\n\t"
        "pushf\n\t"
        "pop %1\n\t"
        : "=&r"(saved), "=r"(restored));
    if (saved != restored) {
        printf("FAIL: [smoke] pushf/popf round-trip changed flags: %lx != %lx\n",
               saved, restored);
        ok = 0;
    }

    if (!ok) return 1;
    printf("OK pushf/pushfq, popf/popfq and invlpg assemble correctly\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
