/* GAS's label-difference idiom "A - B" (both bare numeric-local-label
 * references, e.g. "772b-771b") used in a .byte/.long/.quad data
 * directive — the kernel's ALTERNATIVE() macro (alt_slen/alt_total_slen/
 * alt_rlen in arch/x86/include/asm/alternative.h) computes an
 * instruction-block's length this way, needed for its .altinstructions
 * metadata entries.
 *
 * Before this fix, "772b-771b" wasn't recognized as a label difference
 * at all: it doesn't end in "- ." (so the existing "SYM - ." PC-relative
 * relocation path doesn't match it), and since it starts with a digit
 * it isn't treated as a symbol reference either — it fell through to a
 * plain strtoll() that read the leading digits of the FIRST label name
 * and silently discarded everything after it ("772b-771b" parsed as
 * just "772", truncated to a byte). Both the immediately-resolvable
 * (both labels already defined — "backward" references) and the
 * deferred (at least one still forward-referenced, resolved once
 * assemble_inline finishes) cases are covered here.
 *
 * This only verifies the arithmetic itself (comparing against objdump's
 * view of the assembled bytes) — using real instructions in a
 * .pushsection'd non-.text section (as ALTERNATIVE()'s replacement code
 * actually needs) is a separate, still-open gap: encode_x86()/
 * emit_arm64_branch() currently always target .text regardless of the
 * current section.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* Compile `src`, objdump the given section's raw hex bytes, and check
 * that `expect_hex` (space-separated lowercase hex bytes, as objdump -s
 * prints them) appears somewhere in the output. */
static int check_section_bytes(const char *rcc, const char *td, int pid,
                               const char *tag, const char *src,
                               const char *section, const char *expect_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ld_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_ld_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s", rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j %s %s 2>/dev/null", section, objf);
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

    if (!strstr(out, expect_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" not found in %s:\n%s\n",
               tag, expect_hex, section, out);
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

    /* Both labels already defined ("backward" references) when the
     * .byte directive is processed: 772b-771b = 2 (two 1-byte nops). */
    static const char src_back[] =
        "int main(void) {\n"
        "    __asm__ volatile (\n"
        "        \"771:\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"772:\\n\\t\"\n"
        "        \".pushsection .rodata\\n\\t\"\n"
        "        \".byte 0xAA\\n\\t\"\n"
        "        \".byte 772b - 771b\\n\\t\"\n"
        "        \".byte 0xBB\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "back", src_back, rodata_section_name(), "aa02bb");

    /* Both labels still forward-referenced when the .byte directive is
     * processed, resolved once assemble_inline finishes: 902f-901f = 3
     * (three 1-byte nops). */
    static const char src_fwd[] =
        "int main(void) {\n"
        "    __asm__ volatile (\n"
        "        \".pushsection .rodata\\n\\t\"\n"
        "        \".byte 0xAA\\n\\t\"\n"
        "        \".byte 902f - 901f\\n\\t\"\n"
        "        \".byte 0xBB\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "        \"901:\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"902:\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "fwd", src_fwd, rodata_section_name(), "aa03bb");

    if (!ok) return 1;
    printf("OK\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
