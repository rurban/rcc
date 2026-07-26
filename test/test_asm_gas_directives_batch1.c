/* Four independent GAS-compatibility bugs in the built-in assembler
 * (src/asm.c), all found continuing a real Linux kernel build past the
 * fixes in test_asm_r15d_fixup_reorder.c/test_asm_rel32_stale_skip.c/
 * test_asm_balign_exec_fill.c, chasing objtool errors one at a time on
 * arch/x86/entry/entry_64.S:
 *
 * 1. ".previous" (GAS's single-slot section swap - distinct from the
 *    .pushsection/.popsection stack) was not handled at all, silently
 *    falling through to the "ignored directive" bucket. The kernel's
 *    EXPORT_SYMBOL() macro emits exactly this pattern -
 *    '.section ".export_symbol","a" ; ... ; .previous' - to return to
 *    whatever section was active before it without naming it explicitly.
 *    Left unhandled, every directive after such a block - including the
 *    very next function's own code - stayed wrongly emitted into
 *    ".export_symbol" instead of returning to .text/.entry.text.
 *
 * 2. ".ifb"/".ifnb" (if-blank / if-not-blank macro-parameter tests) were
 *    entirely unrecognized: line_starts_with_dir(p, ".if") requires a
 *    word boundary right after ".if", so ".ifnb"/".ifb" never matched any
 *    of the control-flow dispatch in asm_expand_range(), and the guarded
 *    body between them and their ".endif" was emitted unconditionally
 *    regardless of whether the tested parameter was actually blank. The
 *    kernel's IBRS_ENTER/IBRS_EXIT macros (arch/x86/entry/calling.h) use
 *    ".ifnb \save_reg" to pick between a fast path (arg supplied) and a
 *    slow path (arg omitted) - every call site omitting the optional arg
 *    got the fast-path body anyway.
 *
 * 3. define_label() kept a "shared" ELF symbol-table slot literally named
 *    "1"/"2"/... for every reused GAS numeric local label ("1:") - real
 *    GAS never emits these as real symbols at all (they're purely an
 *    assembler-internal addressing mechanism); only the private,
 *    uniquely-named per-occurrence symbol it created alongside
 *    (.Lrcc_numN.SEQ) is ever consulted by lookup_local()/
 *    lookup_local_sym()/lookup_local_near(). Because skip_insert_shift()
 *    only walks as->locals[] (whose ->sym_idx always points at the
 *    private occurrence symbol), this shared slot's offset went stale -
 *    frozen at wherever the *last* "N:" in the file happened to define it
 *    pre-shift - the moment any later .balign/.skip insertion moved bytes
 *    past it. objtool's decode_instructions() validates every
 *    STT_NOTYPE/STT_FUNC symbol in an executable section names a real
 *    instruction boundary; a stale "1" landing mid-instruction produced
 *    "1(): can't find starting instruction".
 *
 * 4. A same-section *backward* jmp/jcc/call (target already defined) had
 *    its rel32 displacement computed and byte-patched immediately, using
 *    the branch site's *current* buffer position - but a still-pending
 *    FIXUP_SKIP_MAXDIFF/FIXUP_ALIGN between the target and the branch
 *    (e.g. an ALTERNATIVE()'s deferred .skip padding, resolved only in a
 *    separate end-of-file pass) could still shift the branch site forward
 *    without anything going back to fix the already-baked displacement.
 *    Unlike the *forward*-reference case (test_asm_rel32_stale_skip.c),
 *    which already deferred its byte patch via FIXUP_REL32_DEFERRED, the
 *    backward/already-resolved case bypassed that mechanism entirely.
 *    Real kernel case: arch/x86/entry/entry_64.S's asm_load_gs_index -
 *    "jmp 2b" back over an ALTERNATIVE()-generated .skip.
 *
 * Fixed by: adding a ".previous" directive handler (a single prev_sec
 * slot on AsmState, swapped and refreshed by every section-switching
 * directive); recognizing ".ifb"/".ifnb" alongside ".ifc"/".if" in
 * asm_expand_range()'s control-flow dispatch and find_if_branches()'s
 * nested-block depth tracking; skipping the shared ELF symbol entirely
 * for numeric label names; and routing backward-reference jmp/jcc/call
 * through the same FIXUP_REL32_DEFERRED mechanism forward references
 * already used.
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
 * `section`'s objdump -s output. Returns 1 on pass, 0 on fail (printing
 * FAIL with `tag` identifying which sub-case). */
static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *section, const char *want_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_agdb_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_agdb_%s_%d.o", td, tag, pid);

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

/* Compiles `src` and returns 1 if no symbol named *exactly* `sym` (a
 * whole trailing token, not a substring of some longer name) appears
 * anywhere in objdump -t's symbol table; 0 (FAIL) if one does. Mirrors
 * compile_and_get_sym_offset()'s (test_asm_r15d_fixup_reorder.c)
 * whole-token matching, inverted. */
static int compile_and_check_no_symbol(const char *rcc, const char *td, int pid,
                                       const char *tag, const char *src,
                                       const char *sym) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_agdb_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_agdb_%s_%d.o", td, tag, pid);

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

    snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char line[512];
    int found = 0;
    size_t symlen = strlen(sym);
    while (fgets(line, sizeof(line), p)) {
        size_t linelen = strlen(line);
        while (linelen > 0 && isspace((unsigned char)line[linelen - 1])) linelen--;
        if (linelen < symlen || memcmp(line + linelen - symlen, sym, symlen) != 0)
            continue;
        if (linelen != symlen && !isspace((unsigned char)line[linelen - symlen - 1]))
            continue;
        found = 1;
        break;
    }
    pclose(p);
    remove(objf);

    if (found) {
        printf("FAIL: [%s] stale symbol literally named \"%s\" present in symbol table\n",
               tag, sym);
        return 0;
    }
    return 1;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* Bug 1: ".previous" must return to whichever section was active
     * before the intervening ".section" switch, so code after it lands
     * back in .text, not stuck in the throwaway section. */
    static const char src_previous[] =
        ".code64\n.text\n"
        "movl $1, %eax\n"
        ".section \".rcctest_prev\", \"a\"\n"
        ".byte 0xcc\n"
        ".previous\n"
        "movl $2, %eax\n"
        "ret\n";
    ok &= compile_and_check_bytes(rcc, td, pid, "previous_text", src_previous,
                                  ".text", "b801000000b802000000c3");
    ok &= compile_and_check_bytes(rcc, td, pid, "previous_sec", src_previous,
                                  ".rcctest_prev", "cc");

    /* Bug 2: ".ifb"/".ifnb" must actually test whether the substituted
     * macro parameter is blank, not unconditionally emit both arms'
     * combined bodies (which ".ifb"/".ifnb" being no-ops would do). */
    static const char src_ifnb[] =
        ".code64\n.text\n"
        ".macro TESTMAC arg\n"
        ".ifb \\arg\n"
        "movl $222, %eax\n"
        ".endif\n"
        ".ifnb \\arg\n"
        "movl $111, %eax\n"
        ".endif\n"
        ".endm\n"
        "TESTMAC\n"
        "TESTMAC x\n"
        "ret\n";
    ok &= compile_and_check_bytes(rcc, td, pid, "ifnb", src_ifnb,
                                  ".text", "b8de000000b86f000000c3");

    /* Bug 3: a reused numeric local label ("1:") must never leave a
     * shared, literally-"1"-named symbol behind in the real symbol
     * table - only its private per-occurrence .Lrcc_num1.* symbols. */
    static const char src_numlabel[] =
        ".code64\n.text\n"
        ".globl numlabel_fn\n"
        "numlabel_fn:\n"
        "1:\n"
        "nop\n"
        "jmp 1b\n"
        "1:\n"
        "nop\n"
        "jmp 1b\n"
        "ret\n";
    ok &= compile_and_check_no_symbol(rcc, td, pid, "numlabel", src_numlabel, "1");

    /* Bug 4: a same-section *backward* jmp spanning a deferred
     * ALTERNATIVE()-style .skip insertion (target defined before the
     * .skip, jmp encoded after it) must have its displacement patched
     * after, not before, the insertion actually shifts the jmp's own
     * position forward. target_back sits at offset 0 (unaffected by the
     * .skip, which only shifts what comes after it); "movl $42,%eax" (5
     * bytes) is the only real .text content before it, so the correct
     * backward displacement is a fixed, precomputable constant. */
    static const char src_backjmp[] =
        ".code64\n.text\n"
        ".macro altinstr_entry orig, alt, ft_flags, orig_len, alt_len\n"
        "\t.long \\orig-.\n"
        "\t.long \\alt-.\n"
        "\t.4byte \\ft_flags\n"
        "\t.byte \\orig_len\n"
        "\t.byte \\alt_len\n"
        ".endm\n"
        ".macro ALTERNATIVE oldinstr, newinstr, ft_flags\n"
        "740:\n"
        "\t\\oldinstr\n"
        "741:\n"
        "\t.skip -(((744f-743f)-(741b-740b)) > 0) * ((744f-743f)-(741b-740b)),0x90\n"
        "742:\n"
        "\t.pushsection .altinstructions,\"a\"\n"
        "\taltinstr_entry 740b,743f,\\ft_flags,742b-740b,744f-743f\n"
        "\t.popsection\n"
        "\t.pushsection .altinstr_replacement,\"ax\"\n"
        "743:\n"
        "\t\\newinstr\n"
        "744:\n"
        "\t.popsection\n"
        ".endm\n"
        ".globl start_fn\n"
        "start_fn:\n"
        "target_back:\n"
        "\tmovl $42, %eax\n"
        "\tALTERNATIVE \"\", \"nop; nop; nop; nop; nop; nop; nop; nop\", 5\n"
        "\tmovl $99, %eax\n"
        "\tjmp target_back\n";
    /* movl $42,%eax (b82a000000) + 8x 0x90 padding + movl $99,%eax
     * (b863000000) + correct backward jmp (e9e9ffffff, rel32=-23). A
     * stale pre-shift displacement would instead show e9f1ffffff
     * (rel32=-15, computed before the 8-byte insertion). Checked as two
     * separate substrings rather than one 46-hex-char span: objdump -s
     * wraps at 16 bytes/line with its own offset-prefix and ASCII-sidebar
     * columns interleaved between lines, which compile_and_check_bytes's
     * whitespace-collapsing alone doesn't strip - a search string longer
     * than one line's 16 bytes can straddle that boundary and spuriously
     * fail to match even when the real content is correct (jmp's own 5
     * bytes at offset 0x12-0x16 sit safely within the second line alone,
     * offset 0x10-0x1f). */
    ok &= compile_and_check_bytes(rcc, td, pid, "backjmp_setup", src_backjmp, ".text",
                                  "b82a0000009090909090909090b86300");
    ok &= compile_and_check_bytes(rcc, td, pid, "backjmp_disp", src_backjmp, ".text",
                                  "e9e9ffffff");

    if (!ok) return 1;
    printf("OK .previous restores the prior section, .ifb/.ifnb correctly "
           "gate on macro-parameter blankness, reused numeric local labels "
           "leave no stale shared symbol, and a backward jmp spanning a "
           "deferred .skip gets its displacement patched after the "
           "insertion\n");
    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
