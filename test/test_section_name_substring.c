/* `.section`/`.pushsection NAME[, "FLAGS"...]` decided whether NAME was one
 * of the built-in .text/.data/.bss/.rodata sections with a plain
 * strstr(args, ".text") (etc.) check — a *substring* match, not an exact
 * one. Any custom section name that merely *contains* ".text"/".data"/
 * ".bss"/".rodata" anywhere in it — not just a name that literally *is*
 * one of those — got silently folded into the built-in section instead of
 * getting its own, corrupting whatever real code/data happened to sit at
 * the same offset in the wrongly-shared section.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/static_call.h's
 * ARCH_DEFINE_STATIC_CALL_TRAMP macro (used by every DEFINE_STATIC_CALL)
 * does `.pushsection .static_call.text, "ax"` — ".static_call.text"
 * contains ".text" as a substring, so every static-call trampoline landed
 * in the *real* .text section, interleaved with ordinary compiled
 * functions, instead of the dedicated .static_call.text section objtool
 * expects to find them in. objtool's static-call validation pass, unable
 * to locate its trampolines where the "911:" annotation label said they'd
 * be, failed with "can't find starting instruction" — several steps
 * removed from the actual cause (a section-name comparison bug, not
 * anything wrong with the trampoline bytes or the annotation relocation
 * themselves, both of which were already correct).
 *
 * Fixed by comparing the section name's leading comma-separated field
 * *exactly* (section_name_is()) instead of searching the whole argument
 * string as a substring.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int check_section_exists(const char *rcc, const char *td, int pid,
                                const char *tag, const char *src,
                                const char *section) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_sns_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_sns_%s_%d.o", td, tag, pid);

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

    snprintf(cmd, sizeof(cmd), "objdump -h %s " NULL_REDIRECT, objf);
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

    if (!strstr(out, section)) {
        printf("FAIL: [%s] expected a distinct %s section, got:\n%s\n",
               tag, section, out);
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

    /* The exact real-world trigger: a static-call trampoline pushed into
     * ".static_call.text" — the name contains ".text" as a substring but
     * must land in its own section, not the real .text. */
    static const char src_static_call[] =
        "void real_target(void) {}\n"
        "asm(\".pushsection .static_call.text, \\\"ax\\\"\\n\"\n"
        "    \".globl __SCT__my_trampoline\\n\"\n"
        "    \"__SCT__my_trampoline:\\n\"\n"
        "    \".byte 0xe9\\n\"\n"
        "    \".long real_target - (. + 4)\\n\"\n"
        "    \".byte 0x0f, 0xb9, 0xcc\\n\"\n"
        "    \".popsection\\n\");\n"
        "int main(void) { return 0; }\n";
    ok &= check_section_exists(rcc, td, pid, "static_call", src_static_call,
                               ".static_call.text");

    /* Same shape for .data/.bss/.rodata: a suffix match must not collapse
     * a distinctly-named section into the built-in one. */
    static const char src_data[] =
        "asm(\".pushsection .data.once, \\\"aw\\\"\\n\"\n"
        "    \"my_once_data: .long 42\\n\"\n"
        "    \".popsection\\n\");\n"
        "int main(void) { return 0; }\n";
    ok &= check_section_exists(rcc, td, pid, "data_once", src_data, ".data.once");

    static const char src_bss[] =
        "asm(\".pushsection .bss..page_aligned, \\\"aw\\\"\\n\"\n"
        "    \"my_page_bss: .zero 8\\n\"\n"
        "    \".popsection\\n\");\n"
        "int main(void) { return 0; }\n";
    ok &= check_section_exists(rcc, td, pid, "bss_page", src_bss,
                               ".bss..page_aligned");

    static const char src_rodata[] =
        "asm(\".pushsection .rodata.str1.1, \\\"aMS\\\", @progbits, 1\\n\"\n"
        "    \"my_str: .asciz \\\"hi\\\"\\n\"\n"
        "    \".popsection\\n\");\n"
        "int main(void) { return 0; }\n";
    ok &= check_section_exists(rcc, td, pid, "rodata_str", src_rodata,
                               ".rodata.str1.1");

    if (!ok) return 1;
    printf("OK .section/.pushsection compares names exactly, not by substring\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
