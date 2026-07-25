/* Two independent x86-64 assembler bugs, both found compiling the real
 * Linux kernel's arch/x86/entry/entry_64.S (its xen_error_entry function,
 * whose SYM_CODE_START(xen_error_entry) landed at a non-16-aligned address
 * with garbage NOP bytes spliced into the middle of an already-emitted
 * instruction - objtool reported "xen_error_entry(): can't find starting
 * instruction"):
 *
 * 1. parse_x86_reg64() (src/asm.c) checks every r8..r15 GPR name against
 *    its "d"/"w"/"b" (32/16/8-bit) suffixed spellings except r15, whose
 *    entry listed "r15" itself twice instead of including "r15d". Every
 *    operand written as "%r15d" therefore failed to resolve to X86_R15 at
 *    all, silently dropping the REX.R/REX.B prefix that register 15
 *    requires - "xorl %r15d,%r15d" assembled as "31 ff" (xor %edi,%edi:
 *    wrong register, wrong bytes) instead of the correct "45 31 ff". r8d
 *    through r14d were unaffected. The kernel's CLEAR_REGS macro (used by
 *    every PUSH_AND_CLEAR_REGS-based entry stub, including
 *    xen_error_entry) zeroes %r15d among its other clobbered registers.
 *
 * 2. define_label() (src/asm.c) resolves and removes each FIXUP_REL32/
 *    ARM64_B26/PCREL_DATA fixup targeting the label just defined via
 *    "as->fixups[i] = as->fixups[--as->nfixups]" (swap the last array
 *    element into the freed slot) - a classic O(1) removal, but it silently
 *    relies on fixup array order carrying no meaning. It does: every
 *    .balign/.skip's deferred FIXUP_ALIGN/FIXUP_SKIP_MAXDIFF entry is
 *    resolved later, in a *separate* pass that walks the fixup array
 *    strictly left-to-right and assumes array index tracks chronological
 *    (source) order - "the labels/fixups a given insertion must still
 *    shift are exactly the ones after it in the array". A same-array
 *    swap-remove breaks that invariant the moment the removed slot isn't
 *    the last one: whatever fixup used to be last (quite possibly a
 *    FIXUP_ALIGN or FIXUP_SKIP_MAXDIFF created far later in the file,
 *    nowhere near the removed slot) gets spliced into an *earlier* array
 *    position, so it resolves out of turn and applies its own padding
 *    insertion's shift to fixups/labels chronologically *before* it that
 *    have nothing to do with it - silently desyncing every symbol
 *    address and, worse, splicing raw NOP filler bytes into the middle of
 *    already-emitted instructions after it.
 *
 * Fixed by adding the missing "r15d" spelling, and by replacing the
 * swap-with-last removal with an order-preserving memmove.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* Assembles `src` as a standalone .S file and checks that `want_hex`
 * appears (as a contiguous, whitespace-collapsed hex substring) in
 * `objdump -s -j <section>`'s output — the same technique
 * test_asm_macro_gas_features.c uses, which (unlike readelf) understands
 * both ELF and PE/COFF object output, so this works unchanged under a
 * mingw-cross rcc.exe too. */
static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *section, const char *want_hex)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_arfr_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_arfr_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
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

    snprintf(cmd, sizeof(cmd), "objdump -s -j %s %s " NULL_REDIRECT, section, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char out[8192];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[8192];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = (char)tolower((unsigned char)*s);
    collapsed[cn] = '\0';

    if (!strstr(collapsed, want_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" in %s, got:\n%s\n",
               tag, want_hex, section, out);
        return 0;
    }
    return 1;
}

/* Returns `sym`'s section-relative address parsed from `objdump -t`'s
 * symbol table, or -1 if not compiled/found. objdump -t (unlike
 * readelf -s) reads both ELF and PE/COFF object files, so this works
 * unchanged under mingw-cross too - but the two formats disagree on
 * where the address sits on the line: ELF prints it as the very first
 * (bare, no "0x") token -
 *   "0000000000000040 l     O .text  0000000000000000 early_fn"
 * (the size field right before the name is a red herring, always 0
 * here) - while COFF/PE prints an "0x"-prefixed address as the token
 * immediately preceding the name and nothing useful at line start -
 *   "[  2](sec  1)(fl 0x00)(ty    0)(scl   3) (nx 0) 0x0000000000000040 early_fn"
 * (an earlier "(fl 0x00)" field also starts with "0x", so this can't
 * just take the first "0x..." substring on the line either). Branch on
 * whether the line opens with COFF's "[" symbol-index marker. */
static long compile_and_get_sym_offset(const char *rcc, const char *td, int pid,
                                       const char *tag, const char *src, const char *sym)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_arfr_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_arfr_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return -1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return -1; }
    char line[512];
    long off = -1;
    size_t symlen = strlen(sym);
    while (fgets(line, sizeof(line), p)) {
        /* Match the symbol name as a whole trailing token (not a
         * substring of some longer name) at the end of the line. */
        size_t linelen = strlen(line);
        while (linelen > 0 && isspace((unsigned char)line[linelen - 1])) linelen--;
        if (linelen < symlen || memcmp(line + linelen - symlen, sym, symlen) != 0 ||
            (linelen != symlen && !isspace((unsigned char)line[linelen - symlen - 1])))
            continue;
        unsigned long v;
        if (line[0] == '[') {
            /* COFF: the token immediately before the symbol name. */
            const char *q = line + linelen - symlen;
            while (q > line && isspace((unsigned char)q[-1])) q--;
            while (q > line && !isspace((unsigned char)q[-1])) q--;
            if (sscanf(q, "%lx", &v) == 1) { off = (long)v; break; }
        } else {
            /* ELF: the first token on the line. */
            if (sscanf(line, "%lx", &v) == 1) { off = (long)v; break; }
        }
    }
    pclose(p);
    remove(objf);
    return off;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* Bug 1: %r15d in every operand position (dst, src, both-same) plus
     * every other extended GPR for contrast (r8d-r14d were already
     * correct - this isolates r15d specifically). Each xorl needs REX.R
     * and/or REX.B: "45" (both), "44" (reg field only), "41" (rm field
     * only). Losing that prefix doesn't just drop a byte - it silently
     * retargets the instruction onto register 7 (edi/rdi) instead of 15. */
    static const char src_r15d[] =
        ".code64\n.text\n"
        "xorl %r15d, %eax\n"      /* 44 31 f8 */
        "xorl %eax, %r15d\n"      /* 41 31 c7 */
        "movl %r15d, %r15d\n"     /* 45 89 ff */
        "xorl %r14d, %r14d\n"     /* 45 31 f6 (must stay correct) */
        "xorl %r15d, %r15d\n";    /* 45 31 ff */
    ok &= compile_and_check_bytes(rcc, td, pid, "r15d_a", src_r15d, ".text", "4431f8");
    ok &= compile_and_check_bytes(rcc, td, pid, "r15d_b", src_r15d, ".text", "4131c7");
    ok &= compile_and_check_bytes(rcc, td, pid, "r15d_c", src_r15d, ".text", "4589ff");
    ok &= compile_and_check_bytes(rcc, td, pid, "r15d_d", src_r15d, ".text", "4531f6");
    ok &= compile_and_check_bytes(rcc, td, pid, "r15d_e", src_r15d, ".text", "4531ff");

    /* Bug 2: a forward "jmp" whose target label is only defined after a
     * second, much-later .balign - resolving the jmp mid-stream forces
     * define_label() to remove its own FIXUP_REL32 from the fixup array,
     * and (pre-fix) that swap-with-last removal spliced the *later*
     * .balign's deferred FIXUP_ALIGN into an *earlier* array slot,
     * making it resolve before (and wrongly shift) the earlier .balign.
     * Both symbols must land exactly 64-byte aligned. */
    static const char src_reorder[] =
        ".code64\n.text\n"
        "jmp target1\n"
        ".balign 64, 0x90\n"
        "early_fn:\n"
        "movl $111, %eax\n"
        ".fill 200, 1, 0x90\n"
        ".balign 64, 0x90\n"
        "late_fn:\n"
        "movl $222, %eax\n"
        "target1:\n"
        "movl $333, %eax\n"
        "ret\n";
    long early_off = compile_and_get_sym_offset(rcc, td, pid, "reorder1", src_reorder, "early_fn");
    long late_off = compile_and_get_sym_offset(rcc, td, pid, "reorder2", src_reorder, "late_fn");
    if (early_off < 0 || (early_off % 64) != 0) {
        printf("FAIL: [reorder] early_fn at %ld is not 64-byte aligned "
               "(a later .balign's fixup got spliced ahead of it)\n", early_off);
        ok = 0;
    }
    if (late_off < 0 || (late_off % 64) != 0) {
        printf("FAIL: [reorder] late_fn at %ld is not 64-byte aligned\n", late_off);
        ok = 0;
    }

    if (!ok) return 1;
    printf("OK %%r15d resolves with its REX prefix in every operand position, "
           "and resolved fixups no longer reorder later .balign/.skip entries "
           "ahead of earlier ones\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
