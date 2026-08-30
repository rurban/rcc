/* Real GNU AS defaults every untyped label to STT_NOTYPE, regardless of
 * which section it's defined in -- it never auto-infers STT_OBJECT for
 * a plain label either; only an explicit `.type name, @object` directive
 * does that. rcc's standalone assembler defaulted every untyped label to
 * STT_OBJECT instead, wrongly typing bare code labels (e.g. perlasm-
 * generated `.Lloop:`/`.Lprologue:` local labels inside a function body,
 * with no `.type` directive at all) as STT_OBJECT in the final ELF
 * symbol table.
 *
 * This has no effect on the actual compiled machine code (ELF symbol
 * type isn't consulted at runtime), but it is a real correctness defect
 * for any STT-aware tool: `objdump -d` (and other disassemblers/
 * debuggers) treat an STT_OBJECT-typed symbol as data and render the
 * code from that label onward as opaque raw hex instead of decoding
 * instructions.
 *
 * Found via a real OpenSSL build: crypto/sha/sha512-x86_64.s's hand-
 * labeled loop bodies (.Lprologue/.Lloop) desynced objdump's
 * disassembly of the entire rest of the function.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_lbl_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_lbl_%d.o", td, pid);

    static const char src[] =
        ".text\n"
        ".globl main\n"
        "main:\n"
        "    movq $0, %rax\n"
        ".Lloop:\n"     /* bare local label, no .type directive */
        "    incq %rax\n"
        "    cmpq $3, %rax\n"
        "    jl .Lloop\n"
        "    ret\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: assemble failed (rc=%d)\n", rc);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "readelf -s %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    char out[4096] = {0};
    if (p) {
        size_t n = fread(out, 1, sizeof(out) - 1, p);
        out[n] = '\0';
        pclose(p);
    }
    remove(objf);

    char *line = strstr(out, "Lloop");
    if (!line) {
        printf("FAIL: .Lloop symbol not found in symbol table:\n%s\n", out);
        return 1;
    }
    /* Walk back to the start of this symbol table row to check its type field. */
    while (line > out && line[-1] != '\n') line--;
    if (strstr(line, "OBJECT") && (strstr(line, "OBJECT") < strchr(line, '\n'))) {
        printf("FAIL: .Lloop wrongly typed OBJECT (should be NOTYPE), row: %.60s\n", line);
        return 1;
    }

    printf("OK\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
