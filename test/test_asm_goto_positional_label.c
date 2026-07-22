/* GCC's "asm goto" numbers its trailing goto-labels *continuing* the same
 * sequence as the input/output operands — with N in/out operands, the first
 * goto label is referenced in the template as "%lN" (not "%l0"). rcc's
 * template-substitution code recognized the bracketed named form "%l[name]"
 * but, for the plain numbered form "%lN", fell straight through to the
 * ordinary "%N operand" branch, which treated N as an operand index, found
 * it out of range (N >= the operand count), and silently emitted *nothing*
 * for it — dropping the label reference from the assembled text entirely,
 * with no error.
 *
 * Found via a real Linux kernel build: kernel/exit.c's unsafe_put_user()
 * calls expand through arch/x86/include/asm/uaccess.h's __put_user_goto(),
 * whose template is
 *   "1:\tmov"itype" %0,%1\n" _ASM_EXTABLE_UA(1b, %l2)
 * with 2 plain operands (%0, %1) and one goto label referenced as "%l2".
 * _ASM_EXTABLE_UA expands to a ".long (%l2) - ." data directive in a
 * "__ex_table" section entry; with the label silently dropped, that field
 * had no relocation at all — a stray, meaningless empty symbol — which
 * later made objtool fail with "special: can't find new instruction",
 * several steps removed from the actual missing substitution.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* Control-flow correctness: with 2 dummy input operands ahead of it, the
 * goto label must be referenced as %l2 and the jump must actually land on
 * the intended C label. */
static int positional_goto_jumps_correctly(void)
{
    int a = 1, b = 2;
    asm goto("jmp %l2" : : "r"(a), "r"(b) : : mylabel);
    return 0; /* wrong: fell through instead of jumping */
mylabel:
    return 1;
}

/* Data-directive correctness (the exact real-world shape): %l2 used inside
 * a ".long (%l2) - ." directive, mirroring _ASM_EXTABLE_UA(1b, %l2), must
 * produce a real relocation against the C label's alias symbol — not an
 * empty/dropped one. */
static int check_extable_style_reloc(const char *rcc, const char *td, int pid)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_agpl_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_agpl_%d.o", td, pid);

    static const char src[] =
        "static inline int f(int x, int *addr) {\n"
        "    asm goto(\"\\n\"\n"
        "        \"1:\\tmovl %0, %1\\n\"\n"
        "        \".pushsection myextable, \\\"a\\\"\\n\\t\"\n"
        "        \".balign 4\\n\\t\"\n"
        "        \".long (1b) - .\\n\\t\"\n"
        "        \".long (%l2) - .\\n\\t\"\n"
        "        \".popsection\\n\"\n"
        "        : : \"r\"(x), \"m\"(*addr) : : Efault);\n"
        "    return 0;\n"
        "Efault:\n"
        "    return -1;\n"
        "}\n"
        "int main(void) { int v; return f(1, &v); }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [extable_reloc] compile failed (rc=%d)\n", rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "readelf -r %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [extable_reloc] readelf failed to run\n"); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* The second field's relocation must resolve to the real goto-label
     * alias symbol, not be missing or point at an empty/zero symbol. */
    if (!strstr(out, ".L.label.f.Efault")) {
        printf("FAIL: [extable_reloc] expected a relocation against "
               ".L.label.f.Efault, got:\n%s\n", out);
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

    if (positional_goto_jumps_correctly() != 1) {
        printf("FAIL: [control_flow] %%l2 did not jump to the intended label\n");
        ok = 0;
    }
    ok &= check_extable_style_reloc(rcc, td, pid);

    if (!ok) return 1;
    printf("OK positional \"asm goto\" %%lN label references resolve correctly\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
