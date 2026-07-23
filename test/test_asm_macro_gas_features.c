/* Several real GAS `.macro` features that kernel headers rely on constantly
 * were either unimplemented or silently mishandled by rcc's own built-in
 * assembler's macro pre-pass (asm.c). All found chasing a real Linux kernel
 * build failure on arch/x86/entry/entry.S, which pulls in a long chain of
 * `.macro`-heavy headers (include/linux/objtool.h,
 * arch/x86/include/asm/{unwind_hints,nospec-branch,alternative}.h):
 *
 * 1. ".short" (a standard GAS alias for ".2byte"/".hword") wasn't in the
 *    data-emission directive dispatch at all, so it silently emitted zero
 *    bytes for the whole directive — no warning, no error — desyncing every
 *    later byte offset in the section. objtool.h's UNWIND_HINT macro uses
 *    exactly this ("`.short \sp_offset`") to build its ORC unwind-hint
 *    struct.
 *
 * 2. A ".macro NAME param=DEFAULT" parameter's default value was parsed and
 *    then discarded entirely (only the parameter *name* was kept) — a
 *    caller that omitted that parameter, relying on its default, got an
 *    unsubstituted literal "\param" left in the body instead of the
 *    default's actual value. objtool.h's UNWIND_HINT macro
 *    ("type:req sp_reg=0 sp_offset=0 signal=0") is always invoked supplying
 *    only some of its four parameters.
 *
 * 3. GAS accepts whitespace alone between "NAME=VALUE" keyword arguments in
 *    a macro invocation — a comma isn't required between them (only
 *    between a keyword arg and a *positional* one, or between two
 *    positional ones). rcc's invocation-argument scanner split only on
 *    comma, so "UNWIND_HINT sp_reg=ORC_REG_SP sp_offset=8
 *    type=UNWIND_HINT_TYPE_FUNC" (unwind_hints.h's UNWIND_HINT_FUNC body,
 *    no commas at all) was read as ONE argument named "sp_reg" whose value
 *    was the literal text "ORC_REG_SP sp_offset=8 type=UNWIND_HINT_TYPE_FUNC"
 *    — every other parameter silently got its default (or nothing) instead
 *    of its real value.
 *
 * 4. A macro argument written as a quoted string constant (e.g.
 *    nospec-branch.h's ALTERNATIVE_2 invocations passing
 *    "call write_ibpb" as the replacement-instruction argument) keeps its
 *    quote marks as literal bytes when substituted into the macro body,
 *    per GAS's own semantics quotes should be stripped, leaving the plain
 *    instruction text `call write_ibpb` to actually assemble. Left
 *    unstripped, the body tries to assemble the 18-character token
 *    `"call`...`write_ibpb"` (quotes included) instead of a real `call`
 *    instruction, and separately confuses comma-based argument splitting
 *    for any *later* argument in the same invocation.
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
                                   const char *section, const char *want_hex)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_amg_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_amg_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc -D__ASSEMBLY__ " NULL_REDIRECT,
             rcc, objf, srcf);
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
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[4096];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = *s;
    collapsed[cn] = '\0';

    if (!strstr(collapsed, want_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" in %s, got:\n%s\n",
               tag, want_hex, section, out);
        return 0;
    }
    return 1;
}

/* Checks the raw assembled bytes rather than disassembly text — objdump's
 * disassembler sometimes declines to split adjacent, tightly-packed
 * single-byte-aligned symbols (as this test's minimal 6-byte .text
 * deliberately is) into per-instruction mnemonic lines, dumping the raw
 * bytes verbatim instead; the bytes themselves are still the ground truth. */
static int compile_and_check_bytes_text(const char *rcc, const char *td, int pid,
                                        const char *tag, const char *src,
                                        const char *want_hex)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_amg_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_amg_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc -D__ASSEMBLY__ " NULL_REDIRECT,
             rcc, objf, srcf);
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
        if (!isspace((unsigned char)*s)) collapsed[cn++] = *s;
    collapsed[cn] = '\0';

    if (!strstr(collapsed, want_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" in .text, got:\n%s\n", tag, want_hex, out);
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

    /* Bugs 1+2+3 together: the exact real-world shape (simplified) of
     * objtool.h's UNWIND_HINT plus unwind_hints.h's UNWIND_HINT_FUNC — a
     * required param, three defaulted params, invoked with whitespace- (not
     * comma-) separated keyword args, omitting the one relying on its
     * default. */
    static const char src_unwind[] =
        ".macro UNWIND_HINT type:req sp_reg=0 sp_offset=0 signal=0\n"
        "\t.pushsection .discard.unwind_hints\n"
        "\t\t.long 0\n"
        "\t\t.short \\sp_offset\n"
        "\t\t.byte \\sp_reg\n"
        "\t\t.byte \\type\n"
        "\t\t.byte \\signal\n"
        "\t\t.balign 4\n"
        "\t.popsection\n"
        ".endm\n"
        ".macro UNWIND_HINT_FUNC\n"
        "\tUNWIND_HINT sp_reg=7 sp_offset=8 type=2\n"
        ".endm\n"
        ".text\n"
        "foo:\n"
        "\tUNWIND_HINT_FUNC\n"
        "\tret\n";
    /* .long placeholder(4)=00000000, .short sp_offset=8 -> 0800,
     * .byte sp_reg=7 -> 07, .byte type=2 -> 02, .byte signal=0 (DEFAULT,
     * never supplied by the caller) -> 00, padded to align 4 (12 bytes
     * total; no further padding needed since 9 already rounds up to 12). */
    ok &= compile_and_check_bytes(rcc, td, pid, "unwind",
                                  src_unwind, ".discard.unwind_hints",
                                  "000000000800070200000000");

    /* Bug 4: a quoted-string macro argument must have its instruction text
     * assembled for real (a "call" to a real symbol), not left as literal
     * quote-delimited garbage. Mirrors nospec-branch.h's
     * ALTERNATIVE_2 "", "call write_ibpb", \\ibpb_feature, ... shape,
     * simplified to a single always-taken replacement. */
    static const char src_quoted[] =
        ".macro EMIT_ALT newinstr\n"
        "\t\\newinstr\n"
        ".endm\n"
        "write_ibpb:\n"
        "\tret\n"
        ".text\n"
        "foo:\n"
        "\tEMIT_ALT \"call write_ibpb\"\n"
        "\tret\n";
    /* write_ibpb: ret (c3, offset 0). foo: call write_ibpb (e8 + rel32
     * displacement -6, since the call's own 5 bytes start at offset 1 and
     * the next instruction — where the PC-relative displacement is counted
     * from — is at offset 6); ret (c3). Unstripped quote marks around the
     * argument would corrupt this into something other than a real call. */
    ok &= compile_and_check_bytes_text(rcc, td, pid, "quoted", src_quoted, "e8faffffffc3");

    if (!ok) return 1;
    printf("OK GAS .macro: .short directive, default parameter values, "
           "whitespace-separated keyword args, and quoted string arguments\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
