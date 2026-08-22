/* GCC extended-asm templates are ordinary adjacent string literals,
 * concatenated at parse time just like any other C string (a real
 * inline-asm block can spread its template across many short literal
 * lines for readability). parse_asm_stmt() (src/parser.c) concatenated
 * them into a fixed 4096-byte stack buffer, silently DROPPING every
 * token once the running length crossed that boundary -- the length
 * check gated the copy, but `tok` still advanced regardless, so a
 * template whose overflow point fell early enough was effectively
 * emptied, turning the whole inline-asm statement into a silent no-op.
 * codegen.c's own %N-substitution buffers (`adj`/`out`) had the exact
 * same fixed 4096-byte limit one step later.
 *
 * Found via xz's LZMA1 range decoder: rc_asm_bittree()'s hand-unrolled
 * 8-level bittree macro alone needs several thousand bytes once its
 * eight expansions are concatenated, comfortably exceeding 4096.
 *
 * Fixed by sizing parse_asm_stmt()'s concatenation buffer to the exact
 * summed literal length (two-pass: sum first, then copy into an
 * arena-allocated buffer of that exact size) and scaling codegen.c's
 * substitution buffers to the actual template length instead of a
 * fixed cap.
 *
 * This test uses 900 adjacent "nop\n\t" string-literal fragments
 * (4500 bytes concatenated, comfortably over the old 4096 limit) and
 * counts the assembled nop bytes in the resulting object file --
 * pre-fix, the template was silently emptied (0 nops); post-fix, all
 * 900 survive.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"
#define NOP5 "nop\\n\\t"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[160], objf[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_altmpl_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_altmpl_%d.o", td, pid);

    /* 900 separate "nop\n\t" string-literal tokens, one per line --
     * exactly the shape parse_asm_stmt()'s literal-concatenation loop
     * walks (each remains its own TK_STR token; the C-level adjacent-
     * literal concatenation this test exercises happens in the parser,
     * not in the preprocessor -- macro-pasted literal tokens merge
     * earlier and would exercise a different code path). Well over the
     * old fixed 4096-byte buffer once concatenated (4500 bytes). */
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs("int main(void) {\n    __asm__ volatile(\n", f);
    for (int i = 0; i < 900; i++)
        fputs("        \"" NOP5 "\"\n", f);
    fputs("        ::: \"memory\"\n    );\n    return 0;\n}\n", f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -d %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 1; }
    /* 900 "nop\n\t" fragments make for a large disassembly (900 lines);
     * count occurrences of the standalone "nop" mnemonic incrementally
     * rather than buffering the whole thing. */
    int nop_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), p)) {
        /* objdump -d prints one instruction per line; a bare 1-byte nop
         * disassembles as "90                   \tnop". Match the
         * mnemonic as a whole word to avoid counting "nopl"/"nopw"
         * (multi-byte alignment nops the assembler never emits here). */
        const char *m = strstr(line, "\tnop");
        if (m && (m[4] == '\n' || m[4] == '\0'))
            nop_count++;
    }
    pclose(p);
    remove(objf);

    if (nop_count != 900) {
        printf("FAIL: expected 900 assembled \"nop\" instructions from the "
               "concatenated template, found %d (template was truncated "
               "or emptied)\n", nop_count);
        return 1;
    }

    printf("OK: 900-fragment (4500-byte) concatenated inline-asm template "
           "assembled in full, no truncation\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
