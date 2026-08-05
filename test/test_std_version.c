/* -std= must be reflected in the __STDC_VERSION__ predefined macro.
 * gcc_predefined.h bakes it at the C23 value; the handler only overrode
 * it for c99/c11/c17/c23 and left C89/C90 (which must have NO
 * __STDC_VERSION__) still seeing 202311L. A -std=c89 build then took
 * C23-only header branches -- notably rcc's own <stddef.h>
 * `typedef typeof(nullptr) nullptr_t;` -- and failed to parse, so a
 * plain `#include <stddef.h>` broke under -std=c89 (e.g. parson). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

/* Compile `body' with the given -std flag; return rcc's exit status. */
static int compile_with(const char *rcc, const char *std, const char *body)
{
    const char *td = get_tmpdir();
    char src[600], obj[600], cmd[3200];
    int pid = (int)getpid();
    snprintf(src, sizeof(src), "%s/test_stdver_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_stdver_%d.o", td, pid);
    FILE *sf = fopen(src, "w");
    if (!sf) return -1;
    fputs(body, sf);
    fclose(sf);
    snprintf(cmd, sizeof(cmd), "%s -std=%s -c -o %s %s %s",
             rcc, std, obj, src, NULL_REDIRECT);
    int rc = system(cmd);
    remove(src);
    remove(obj);
    return rc;
}

int main(void)
{
    const char *rcc = find_rcc();

    /* C89/C90: __STDC_VERSION__ must be undefined, and <stddef.h> must
     * still parse (its nullptr_t C23 branch must be gated out). */
    const char *c89_body =
        "#ifdef __STDC_VERSION__\n#error has_stdc_version\n#endif\n"
        "#include <stddef.h>\nint main(void){return 0;}\n";
    if (compile_with(rcc, "c89", c89_body) != 0) {
        printf("FAIL: -std=c89 defines __STDC_VERSION__ or breaks <stddef.h>\n");
        return 1;
    }

    /* c99 / c11 / c17 must set the exact standard values. */
    struct { const char *std; const char *ver; } cases[] = {
        {"c99", "199901L"}, {"c11", "201112L"}, {"c17", "201710L"},
    };
    for (int i = 0; i < 3; i++) {
        char body[256];
        snprintf(body, sizeof(body),
                 "#if __STDC_VERSION__ != %s\n#error wrong_version\n#endif\n"
                 "int main(void){return 0;}\n", cases[i].ver);
        if (compile_with(rcc, cases[i].std, body) != 0) {
            printf("FAIL: -std=%s did not set __STDC_VERSION__=%s\n",
                   cases[i].std, cases[i].ver);
            return 2;
        }
    }

    printf("OK\n");
    return 0;
}
