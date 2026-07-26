/* The Linux kernel's ALTERNATIVE() macro (arch/x86/include/asm/
 * alternative.h) pads the original instruction to at least the
 * replacement's length with ".skip -(((RLEN)-(SLEN)) > 0) * ((RLEN)-
 * (SLEN)),0x90" — i.e. max(0, replacement_len - original_len) NOP bytes —
 * where RLEN/SLEN are themselves GAS label-differences (e.g. "775f-774f"),
 * and RLEN's labels live in a *later*, separate .pushsection'd
 * .altinstr_replacement block, making them forward references at the
 * point the .skip is processed.
 *
 * Before this fix, rcc's ".skip"/".zero"/".space" handling was a bare
 * atoi() of the directive's argument text; fed this arithmetic expression
 * it read the leading "-" and then a non-digit "(", returning 0 — so the
 * padding was *always* skipped. That's silently correct only when the
 * original instruction happens to already be at least as long as the
 * replacement (a common but not universal case); the kernel's clac()/
 * stac() — ALTERNATIVE("", "clac"/"stac", ...), a zero-byte original
 * padded up to a real 3-byte instruction — hit the always-wrong case:
 * objtool flagged the resulting .altinstructions entry as "empty
 * alternative entry" (its length field, itself a label-difference
 * spanning the un-padded gap, came out 0 instead of 3).
 *
 * Fixed by recognizing this one fixed expression shape and deferring it
 * as a FIXUP_SKIP_MAXDIFF, resolved once all four labels are known (end
 * of the same assemble_inline call — the macro always pushes into
 * .altinstr_replacement before returning to .text), inserting the
 * padding bytes and shifting every local label/fixup recorded after that
 * point in the same section.
 *
 * This also exercises two parser bugs found via the same macro (C string
 * literal concatenation glues adjacent pieces onto one physical line with
 * no separating whitespace): a label immediately followed by real code —
 * "1:jmp label" — and a label immediately followed by a comment —
 * "1: # comment" (both from ARCH_STATIC_BRANCH_ASM / OLDINSTR).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static void collapse_hex_dump(const char *out, char *hexbuf, size_t hexbuf_sz) {
    size_t hn = 0;
    const char *line = out;
    while (line && *line && hn + 1 < hexbuf_sz) {
        const char *nl = strchr(line, '\n');
        const char *end = nl ? nl : line + strlen(line);
        const char *p = line;
        while (p < end && isspace((unsigned char)*p)) p++;
        const char *addr_start = p;
        while (p < end && isxdigit((unsigned char)*p)) p++;
        if (p > addr_start && p < end && isspace((unsigned char)*p)) {
            while (p < end && isspace((unsigned char)*p)) p++;
            while (p < end && hn + 1 < hexbuf_sz) {
                if (isxdigit((unsigned char)*p)) {
                    hexbuf[hn++] = *p++;
                } else if (*p == ' ') {
                    if (p + 1 < end && p[1] == ' ') break;
                    p++;
                } else {
                    break;
                }
            }
        }
        line = nl ? nl + 1 : NULL;
    }
    hexbuf[hn] = '\0';
}

static int check_section_bytes(const char *rcc, const char *td, int pid,
                               const char *tag, const char *src,
                               const char *section, const char *expect_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_skipmd_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_skipmd_%s_%d.o", td, tag, pid);

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

    snprintf(cmd, sizeof(cmd), "objdump -s -j %s %s " NULL_REDIRECT, section, objf);
    FILE *p = popen(cmd, "r");
    if (!p) {
        printf("FAIL: [%s] objdump failed to run\n", tag);
        remove(objf);
        return 0;
    }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char hexbuf[4096];
    collapse_hex_dump(out, hexbuf, sizeof(hexbuf));
    if (!strstr(hexbuf, expect_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" not found in %s:\n%s\n",
               tag, expect_hex, section, out);
        return 0;
    }
    return 1;
}

/* Compile `src` and fail if rcc's stderr contains `bad`. Used for the
 * "label: # comment" case: unlike "1:jmp", a mis-parsed trailing comment
 * doesn't corrupt any bytes on its own (the bogus "#" pseudo-instruction
 * just emits zero bytes with a warning) — the only directly observable
 * symptom in isolation is that spurious "unknown x86 instruction: #"
 * warning, so that's what this checks for. */
static int check_no_stderr(const char *rcc, const char *td, int pid,
                           const char *tag, const char *src, const char *bad) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_skipmd_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_skipmd_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s 2>&1", rcc, objf, srcf);
    FILE *p = popen(cmd, "r");
    remove(srcf);
    if (!p) {
        printf("FAIL: [%s] failed to run compiler\n", tag);
        remove(objf);
        return 0;
    }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    if (strstr(out, bad)) {
        printf("FAIL: [%s] unexpected \"%s\" in compiler output:\n%s\n", tag, bad, out);
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

    /* The exact ALTERNATIVE("", "clac", ...) shape: an empty original
     * (771:/772: back to back, 0 bytes) padded up to clac's 3 bytes
     * (0F 01 CA) via the forward-referenced ".skip" expression. The
     * .altinstructions entry's last two bytes are orig_len/repl_len —
     * both must read 3 (the real GAS/objtool-correct value), not 0. */
    static const char src_clac[] =
        "int main(void) {\n"
        "    __asm__ volatile(\n"
        "        \"771:\\n\\t\" \"\" \"\\n772:\\n\\t\"\n"
        "        \".skip -(((775f-774f)-(772b-771b)) > 0) * ((775f-774f)-(772b-771b)),0x90\\n\\t\"\n"
        "        \"773:\\n\\t\"\n"
        "        \".pushsection .altinstructions, \\\"aM\\\", @progbits, 14\\n\\t\"\n"
        "        \" .long 771b - .\\n\\t\"\n"
        "        \" .long 774f - .\\n\\t\"\n"
        "        \" .4byte 0\\n\\t\"\n"
        "        \" .byte 773b-771b\\n\\t\"\n"
        "        \" .byte 775f-774f\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "        \".pushsection .altinstr_replacement, \\\"ax\\\"\\n\\t\"\n"
        "        \"774:\\n\\t\" \"clac\" \"\\n775:\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "        ::: \"memory\");\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "clac_pad", src_clac,
                              ".altinstructions", "0303");
    /* And the padding must actually be *in .text*, as real 0x90 NOP
     * bytes between the (empty) original and 773 — not just an
     * incidentally-correct length field. */
    ok &= check_section_bytes(rcc, td, pid, "clac_nops", src_clac,
                              ".text", "909090");

    /* "1:jmp label" — a label's colon directly followed, with no
     * whitespace, by a real instruction on the same physical line (from
     * "1:" and "jmp %l[...]" landing adjacent via string-literal
     * concatenation in ARCH_STATIC_BRANCH_ASM). Before the fix this
     * silently dropped the jmp entirely (mnem truncated to "1", "jmp"
     * discarded) — nop nop jmp-encoding (EB 00) must appear. */
    static const char src_label_jmp[] =
        "int main(void) {\n"
        "    __asm__ volatile(\n"
        "        \"nop\\n\\t\" \"nop\\n\\t\"\n"
        "        \"1:jmp 2f\\n\\t\"\n"
        "        \"2:\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "label_jmp", src_label_jmp,
                              ".text", "9090e900000000");

    /* "1: # comment" — a label's colon directly followed by a GAS
     * comment on the same physical line (OLDINSTR's "# ALT: oldinstr\n"
     * glued onto a preceding "1: " with no newline between them). Before
     * the fix this fell through to instruction parsing with mnem "#",
     * discarding the same information a plain leading-"#" comment line
     * would have skipped correctly; a real instruction on the next line
     * must still encode correctly right after it. */
    static const char src_label_comment[] =
        "int main(void) {\n"
        "    __asm__ volatile(\n"
        "        \"nop\\n\\t\"\n"
        "        \"1: # a comment\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_no_stderr(rcc, td, pid, "label_comment", src_label_comment,
                          "unknown x86 instruction: #");

    if (!ok) return 1;
    printf("OK ALTERNATIVE() .skip padding + label parsing\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
