/* `.comm name, size[, align]` (uninitialized global data) and `.lcomm`
 * (its file-local counterpart) were entirely unrecognized by rcc's
 * standalone assembler -- silently dropped as an unknown directive,
 * leaving the symbol permanently undefined at link time.
 *
 * Found via a real OpenSSL build: crypto/x86_64cpuid.pl's perlasm output
 * declares `.hidden OPENSSL_ia32cap_P` / `.comm OPENSSL_ia32cap_P,40,4`
 * at file scope (before any .bss/.text switch); every one of
 * OPENSSL_ia32cap_P's many C-file references (cpuid.c, info.c, ...)
 * stayed unresolved at final link ("undefined reference to
 * `OPENSSL_ia32cap_P'").
 *
 * rcc's own C-compiler codegen never emits true SHN_COMMON symbols
 * (tentative definitions always become a concrete .bss allocation, safe
 * whenever only one translation unit provides the real definition -- the
 * overwhelmingly common real-world shape), so the standalone assembler
 * mirrors that: .comm/.lcomm allocate real .bss space immediately rather
 * than emitting a genuine cross-object-file-mergeable common symbol.
 *
 * This test defines a symbol via `.hidden`+`.comm` in one assembled .s
 * file, references it (declared `extern`) from a separately compiled .c
 * file, links both together, and confirms the linked program can read
 * and write through it correctly at the expected size.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char asmf[128], cf[128], exef[128], cmd[900];
    snprintf(asmf, sizeof(asmf), "%s/test_comm_%d.s", td, pid);
    snprintf(cf, sizeof(cf), "%s/test_comm_%d.c", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_comm_%d", td, pid);

    FILE *fa = fopen(asmf, "w");
    if (!fa) { printf("FAIL: cannot write %s\n", asmf); return 1; }
    fputs(".hidden my_comm_sym\n"
          ".comm my_comm_sym,40,4\n",
          fa);
    fclose(fa);

    FILE *fc = fopen(cf, "w");
    if (!fc) { printf("FAIL: cannot write %s\n", cf); remove(asmf); return 1; }
    fputs("#include <stdio.h>\n"
          "extern int my_comm_sym[10];\n"
          "int main(void) {\n"
          "    for (int i = 0; i < 10; i++) my_comm_sym[i] = i * i;\n"
          "    long long sum = 0;\n"
          "    for (int i = 0; i < 10; i++) sum += my_comm_sym[i];\n"
          "    printf(\"%lld\\n\", sum);\n"
          "    return sum != 285;\n"
          "}\n",
          fc);
    fclose(fc);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s %s " NULL_REDIRECT, rcc, exef, cf, asmf);
    int rc = system(cmd);
    remove(asmf);
    remove(cf);
    if (rc != 0) {
        printf("FAIL: build failed (rc=%d)\n", rc);
        return 1;
    }

    rc = system(exef);
    remove(exef);
    if (rc != 0) {
        printf("FAIL: program exited %d (expected sum 285)\n", rc);
        return 1;
    }

    printf("OK\n");
    return 0;
}
