/* -std=c2y / -std=c29 (WG14 renamed C2Y to C29): every spelling
 * (c2y, gnu2y, c29, gnu29, iso9899:2029) must set __STDC_VERSION__ to
 * gcc's own placeholder value (202500L; clang instead uses 202400L --
 * matching gcc since rcc targets gcc compatibility). Also a regression
 * guard: two preprocess.c gates (#if true/false, the C23 `bool` macro)
 * used to compare __STDC_VERSION__ for EXACT equality with the C23
 * value, so they silently went dark under any later standard -- must
 * stay active under c2y/c29 too.
 */
#include "test_common.h"

static int compile_with(const char *rcc, const char *std, const char *body)
{
    const char *td = get_tmpdir();
    char src[600], obj[600], cmd[3200];
    int pid = (int)getpid();
    snprintf(src, sizeof(src), "%s/test_c29_stdver_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_c29_stdver_%d.o", td, pid);
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

    /* Every c2y/c29 spelling sets the same __STDC_VERSION__. */
    const char *stds[] = {"c2y", "gnu2y", "c29", "gnu29", "iso9899:2029"};
    for (size_t i = 0; i < sizeof(stds) / sizeof(stds[0]); i++) {
        char body[256];
        snprintf(body, sizeof(body),
                 "#if __STDC_VERSION__ != 202500L\n#error wrong_version\n#endif\n"
                 "int main(void){return 0;}\n");
        if (compile_with(rcc, stds[i], body) != 0) {
            printf("FAIL: -std=%s did not set __STDC_VERSION__=202500L\n", stds[i]);
            return 1;
        }
    }

    /* c2y must be newer than c23 for every #if version comparison
     * (>=, not the C23 value alone). */
    {
        const char *body =
            "#if __STDC_VERSION__ < 202311L\n#error must_be_at_least_c23\n#endif\n"
            "int main(void){return 0;}\n";
        if (compile_with(rcc, "c2y", body) != 0) {
            printf("FAIL: -std=c2y's __STDC_VERSION__ is not >= C23's\n");
            return 2;
        }
    }

    /* Regression: #if true / #if false must still work under c2y (used
     * to be gated on an EXACT match against the C23 version string). */
    {
        const char *body =
            "#if true\n#else\n#error if_true_broken\n#endif\n"
            "#if false\n#error if_false_broken\n#endif\n"
            "int main(void){return 0;}\n";
        if (compile_with(rcc, "c2y", body) != 0) {
            printf("FAIL: '#if true'/'#if false' broken under -std=c2y\n");
            return 3;
        }
    }

    /* Regression: the C23 `bool` macro predefine must still fire under
     * c2y (a plain identifier `bool` with no <stdbool.h> must work). */
    {
        const char *body = "bool b = 1; int main(void){return (int)b - 1;}\n";
        if (compile_with(rcc, "c2y", body) != 0) {
            printf("FAIL: bare 'bool' identifier broken under -std=c2y\n");
            return 4;
        }
    }

    /* The C29 checklist features themselves must compile under an
     * explicit -std=c2y (they're unconditional in rcc, gated on no std
     * flag at all, but must not be REJECTED under the standard that
     * actually defines them). */
    {
        const char *body =
            "#include <stdcountof.h>\n"
            "int arr[10];\n"
            "int main(void) {\n"
            "    int x = 0o17;\n"
            "    if (_Countof(arr) != 10) return 1;\n"
            "    if (countof(arr) != 10) return 2;\n"
            "    if (x != 15) return 3;\n"
            "    switch (x) { case 10 ... 20: break; default: return 4; }\n"
            "    if (int y = x - 15) { return 5; } else { if (y != 0) return 6; }\n"
            "    return 0;\n"
            "}\n";
        const char *td = get_tmpdir();
        char src[600], exe[600], cmd[3200];
        int pid = (int)getpid();
        snprintf(src, sizeof(src), "%s/test_c29_stdver_full_%d.c", td, pid);
        snprintf(exe, sizeof(exe), "%s/test_c29_stdver_full_%d", td, pid);
        FILE *sf = fopen(src, "w");
        if (!sf) { printf("FAIL: cannot write %s\n", src); return 5; }
        fputs(body, sf);
        fclose(sf);
        snprintf(cmd, sizeof(cmd), "%s -std=c2y -Iinclude -o %s %s %s",
                 rcc, exe, src, NULL_REDIRECT);
        int rc = system(cmd);
        if (rc == 0) {
            snprintf(cmd, sizeof(cmd), "%s", exe);
            rc = system(cmd);
        }
        remove(src);
        remove(exe);
        if (rc != 0) {
            printf("FAIL: C29 feature smoke test failed under -std=c2y (rc=%d)\n", rc);
            return 6;
        }
    }

    return 0;
}
