/* Bare "-M"/"-MM" (dependency-rule-only mode: no compilation at all,
 * just a Make rule printed to stdout/-MF/-o -- distinct from -MD/-MMD,
 * which compile normally AND write a .d file as a side effect) fell
 * through the "ignored unknown option" path: rcc proceeded to a full,
 * normal compile-and-link of the single translation unit, which is
 * wrong on two counts -- no dependency rule was ever printed, and the
 * spurious link (undefined references to every symbol the TU doesn't
 * itself define) corrupted the surrounding build. Kefir's own Makefile
 * generates its .d files with the classic idiom
 * `$(CC) -MM -MT '<obj>' <src> > <dep>` (test/third_party's
 * test_kefir); this hit every single one of its ~800 source files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char src[600], hdr[600], cmd[2600], dep[600];
    int pid = (int)getpid();

    snprintf(src, sizeof(src), "%s/test_depmm_%d.c", td, pid);
    snprintf(hdr, sizeof(hdr), "%s/test_depmm_%d.h", td, pid);
    snprintf(dep, sizeof(dep), "%s/test_depmm_%d.dep", td, pid);

    FILE *hf = fopen(hdr, "w");
    if (!hf) { printf("FAIL: cannot write %s\n", hdr); return 1; }
    fputs("#define ANSWER 42\n", hf);
    fclose(hf);

    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: cannot write %s\n", src); remove(hdr); return 1; }
    fputs("int undefined_symbol_never_linked(void);\n"
          "int main(void){return undefined_symbol_never_linked() + ANSWER;}\n", sf);
    fclose(sf);

    /* -MM -MT target -MF <file>, matching kefir's own idiom
     * ($(CC) -MM -MT '<obj>' <src> > <dep>, here via -MF instead of shell
     * redirection so the test itself stays shell/quoting-independent).
     * The header is pulled in via -include rather than a "#include
     * <path>" directive embedded in the generated source, so a raw
     * Windows backslash path never has to survive being written through
     * a C string literal. No -c, no -o: a real compile-and-link attempt
     * here would fail at link time (undefined_symbol_never_linked is
     * declared but never defined) -- -M/-MM must never reach codegen/
     * link at all. */
    snprintf(cmd, sizeof(cmd), "%s -include %s -MM -MT the_target.o -MF %s %s",
             rcc, hdr, dep, src);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: -MM invocation itself failed (rc=%d) -- likely fell "
               "through to a real compile+link instead of printing a rule\n", rc);
        remove(src); remove(hdr); remove(dep);
        return 2;
    }

    FILE *f = fopen(dep, "r");
    if (!f) { printf("FAIL: no dependency file generated\n"); remove(src); remove(hdr); return 3; }
    char content[4096];
    size_t n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);
    remove(src); remove(hdr); remove(dep);

    if (strncmp(content, "the_target.o:", 13) != 0) {
        printf("FAIL: rule target is not the -MT value: %.60s\n", content);
        return 4;
    }
    {
        const char *src_base = strrchr(src, '/');
        const char *src_base2 = strrchr(src, '\\');
        if (src_base2 && (!src_base || src_base2 > src_base)) src_base = src_base2;
        src_base = src_base ? src_base + 1 : src;
        if (!strstr(content, src_base)) {
            printf("FAIL: source not listed as prerequisite: %.200s\n", content);
            return 5;
        }
    }
    {
        const char *hdr_base = strrchr(hdr, '/');
        const char *hdr_base2 = strrchr(hdr, '\\');
        if (hdr_base2 && (!hdr_base || hdr_base2 > hdr_base)) hdr_base = hdr_base2;
        hdr_base = hdr_base ? hdr_base + 1 : hdr;
        if (!strstr(content, hdr_base)) {
            printf("FAIL: included header not listed as prerequisite: %.200s\n", content);
            return 6;
        }
    }
    /* No object/executable must have been produced -- -M/-MM never compiles. */
    if (access("a.out", F_OK) == 0) {
        printf("FAIL: -MM produced a link output (a.out) -- it must never compile/link\n");
        remove("a.out");
        return 7;
    }

    /* Bare "-M" (system headers included too, unlike real GCC's -MM --
     * rcc doesn't track that split either way, matching its existing
     * -MD/-MMD behavior) must behave the same way, this time with -MF's
     * default-derived target ("a.out", no -MT/-o given here). */
    snprintf(src, sizeof(src), "%s/test_depm_%d.c", td, pid);
    snprintf(dep, sizeof(dep), "%s/test_depm_%d.dep", td, pid);
    sf = fopen(src, "w");
    if (!sf) { printf("FAIL: cannot write %s\n", src); return 8; }
    fputs("int main(void){return 0;}\n", sf);
    fclose(sf);
    snprintf(cmd, sizeof(cmd), "%s -M -MF %s %s", rcc, dep, src);
    rc = system(cmd);
    remove(src);
    if (rc != 0) {
        printf("FAIL: bare -M invocation failed (rc=%d)\n", rc);
        remove(dep);
        return 9;
    }
    f = fopen(dep, "r");
    if (!f) { printf("FAIL: no -M output captured\n"); return 10; }
    n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);
    remove(dep);
    /* Default target is the platform's own default link-output basename
     * (a.out on POSIX, a.exe on Windows) when neither -MT nor -o is given. */
    if ((strncmp(content, "a.out:", 6) != 0 && strncmp(content, "a.exe:", 6) != 0) ||
        !strstr(content, src)) {
        printf("FAIL: -M default target malformed: %.80s\n", content);
        return 11;
    }

    printf("OK\n");
    return 0;
}
