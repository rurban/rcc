/* File-scope asm(...) — outside any function body — went through a
 * completely separate, hand-rolled mini-parser instead of the full
 * assembler used for function-body inline asm: it only understood a
 * single "label: insn" line, with a hardcoded handful of recognized
 * instructions (ret/nop/int3 on x86), silently doing nothing for
 * anything else ("for other instructions, fall through to nothing
 * (assembler needed)", per its own comment).
 *
 * That broke any file-scope asm() beyond the trivial case — most
 * notably the Linux kernel's raw initcall registration (include/linux/
 * init.h's ____define_initcall, used by every *_initcall() macro):
 *   asm(".section \"" __sec "\", \"a\"\n"
 *       __stringify(__name) ":\n"
 *       ".long " __stringify(__stub) " - .\n"
 *       ".previous\n");
 * Since the mini-parser looked for the *first* ':' anywhere in the
 * whole string to split "label" from "insn", and the .section
 * directive line has no colon in it, the label name it extracted was
 * literally the entire ".section ...\nreal_label" text glued together
 * — objdump showed a garbled multi-line symbol name, no
 * .initcallN.init section was ever created, and the real ".long X - ."
 * data was dropped entirely. objtool caught the fallout as "STT_FUNC
 * at end of section" several steps removed from the actual cause.
 *
 * Fixed by routing file-scope asm() through the same assemble_inline()
 * used for function bodies (there's no enclosing function, so no
 * later C-level goto label it could ever need to forward-reference).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int vide_called;
void vide(void);
asm("vide: ret");

typedef int (*initcall_t)(void);
static int myinit(void) { vide_called = 1; return 0; }
static initcall_t __attribute__((used, section(".discard.initcall"))) __stub_myinit = myinit;
asm(".section \".initcall4.init\", \"a\"		\n"
    "__initcall_myinit4:			\n"
    ".long	__stub_myinit - .	\n"
    ".previous					\n");

int main(void)
{
    /* The trivial "label: insn" case must keep working. */
    vide();
    if (vide_called) return 1; /* vide() must be a real ret, not our init func */

    /* The .section/label/.long-with-relocation/.previous case: verify
     * via objdump that .initcall4.init actually exists, holds a real
     * relocation against __stub_myinit (not zero bytes, not dropped),
     * and that the label symbol name is exactly "__initcall_myinit4"
     * — not glued to the preceding .section directive text. */
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_tla_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_tla_%d.o", td, pid);

    static const char src[] =
        "typedef int (*initcall_t)(void);\n"
        "static int myinit(void) { return 0; }\n"
        "static initcall_t __attribute__((used, section(\".discard.initcall\"))) __stub_myinit = myinit;\n"
        "asm(\".section \\\".initcall4.init\\\", \\\"a\\\"\\t\\t\\n\"\n"
        "    \"__initcall_myinit4:\\t\\t\\t\\n\"\n"
        "    \".long\\t__stub_myinit - .\\t\\n\"\n"
        "    \".previous\\t\\t\\t\\t\\t\\n\");\n"
        "int main(void) { return 0; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 2; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s 2>/dev/null", rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 3;
    }

    snprintf(cmd, sizeof(cmd), "objdump -tr %s 2>/dev/null", objf);
    FILE *p = popen(cmd, "r");
    if (!p) {
        printf("FAIL: objdump failed to run\n");
        remove(objf);
        return 4;
    }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    if (!strstr(out, ".initcall4.init")) {
        printf("FAIL: .initcall4.init section missing:\n%s\n", out);
        return 5;
    }
    if (!strstr(out, "__initcall_myinit4")) {
        printf("FAIL: __initcall_myinit4 symbol missing/garbled:\n%s\n", out);
        return 6;
    }
    if (strstr(out, ".section")) {
        printf("FAIL: a symbol/reloc name still contains \".section\" text "
               "(label glued to the directive line):\n%s\n", out);
        return 7;
    }
    if (!strstr(out, pc32_reloc_name()) || !strstr(out, "__stub_myinit")) {
        printf("FAIL: expected a %s relocation against "
               "__stub_myinit, got:\n%s\n", pc32_reloc_name(), out);
        return 8;
    }

    printf("OK file-scope asm() through the full assembler\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
