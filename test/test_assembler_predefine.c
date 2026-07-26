/* Every other C preprocessor automatically predefines __ASSEMBLER__ when
 * processing a ".S"/".s" input (GNU cpp's "assembler-with-cpp" mode) —
 * this is separate from, and older than, the Linux kernel's own
 * Makefile-provided -D__ASSEMBLY__. Headers shared between C and
 * assembly (arch/x86/include/asm/desc_defs.h et al.) gate their C-only
 * content (struct/enum definitions) behind "#ifndef __ASSEMBLER__" —
 * without it predefined, that C-only content leaked straight into the
 * assembler's input as plain text, which naturally can't parse a struct
 * or enum definition and reported it as a cascade of confusing,
 * unrelated errors ("bad register argument", "popsection without
 * pushsection", ...) several steps removed from the real cause.
 *
 * Found via a real Linux kernel build: arch/x86/boot/startup/efi-mixed.S
 * (reachable once standalone .S file compilation itself started working,
 * see test_standalone_asm_file.c) transitively includes
 * arch/x86/include/asm/desc_defs.h, whose "#ifndef __ASSEMBLER__" branch
 * (struct desc_struct, several enums) was reaching the assembler as raw,
 * unparseable text.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_apd_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_apd_%d.o", td, pid);

    /* Mirrors desc_defs.h's real shape: C-only content gated behind
     * "#ifndef __ASSEMBLER__", real assembly gated behind the #else. A
     * pre-fix compile would feed "struct desc_struct { ... };" straight
     * to the assembler as unparseable text. */
    static const char src[] =
        "#ifndef __ASSEMBLER__\n"
        "struct desc_struct {\n"
        "    unsigned short limit0;\n"
        "    unsigned short base0;\n"
        "};\n"
        "enum { GATE_INTERRUPT = 0xE };\n"
        "#else\n"
        ".globl real_label\n"
        "real_label:\n"
        "\tnop\n"
        "#endif\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d) - __ASSEMBLER__ not predefined "
               "for a .S input?\n", rc);
        remove(objf);
        return 2;
    }

    snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 3; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    if (!strstr(out, "real_label")) {
        printf("FAIL: expected real_label symbol (the #else/assembly "
               "branch), got:\n%s\n", out);
        return 1;
    }

    printf("OK __ASSEMBLER__ is predefined for a .S input, gating C-only "
           "content away from the assembler\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
