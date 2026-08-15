/* C11 6.7.6.3p10: a parameter's declared qualified type is taken as its
 * UNQUALIFIED version for function-type compatibility -- a top-level
 * const/volatile/restrict on a by-value parameter (including a
 * function-pointer-typed one) may legally differ between a declaration
 * and its definition; real gcc/clang accept it silently even under
 * -Wall -Wextra. rcc's redeclaration check compared the raw
 * (unstripped) parameter qualifiers and wrongly warned "conflicting
 * type qualifiers", which -W turns into a diagnostic and -Werror
 * promotes to a hard build failure -- e.g. njs's
 * njs_vm_external_constructor(), declared with a plain
 * "njs_function_native_t native" parameter but defined with
 * "const njs_function_native_t native". Only reproduces under -W (the
 * default unit-test build doesn't pass it), so this test drives rcc as
 * a subprocess to exercise the exact flag combination.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char srcf[512], exef[512], cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/rcc_qual_redecl_%d.c", td, (int)getpid());
    snprintf(exef, sizeof(exef), "%s/rcc_qual_redecl_%d.exe", td, (int)getpid());

    FILE *f = fopen(srcf, "w");
    if (!f) {
        printf("FAIL: cannot create temp file\n");
        return 1;
    }
    fputs("typedef int (*fn_t)(int);\n"
          "int caller(fn_t native, int x);\n"
          "int caller(const fn_t native, int x) { return native(x); }\n"
          "static int triple(int x) { return x * 3; }\n"
          "int main(void) { return caller(triple, 4) != 12; }\n",
          f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -W -Werror -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: top-level const parameter redeclaration rejected under -W -Werror (rc=%d)\n", rc);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "%s", exef);
    rc = system(cmd);
    remove(exef);
    if (rc != 0) {
        printf("FAIL: compiled program returned wrong result\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}
