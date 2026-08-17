/* -funsigned-char/-fsigned-char (override plain `char`'s default
 * signedness) were entirely unrecognized -- and, under a bare -Werror
 * (a common combination: e.g. test/third_party's test_camgunz_cmp
 * builds with `-Werror ... -funsigned-char -fwrapv ... --pedantic-errors`),
 * an unrecognized non-warning flag is a hard compile error, not just a
 * tolerated warning. Also missing: `-fwrapv` (rcc's codegen never
 * exploits signed-overflow UB, so it is already always effectively
 * -fwrapv -- just needed accepting as a recognized no-op) and the GNU
 * long-option spelling `--pedantic-errors` (only the single-dash
 * `-pedantic-errors` was recognized). Fixed by: implementing
 * -funsigned-char/-fsigned-char for real (mutates the shared `ty_char`
 * global's `is_unsigned` bit once, before any translation unit is
 * parsed -- rcc is single-TU-at-a-time/native-only, so this is safe for
 * the whole compilation), and accepting -fwrapv/--pedantic-errors.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int compile_and_run(const char *rcc, const char *td, int pid,
                            const char *flags, int *out_val)
{
    char src[600], bin[600], cmd[2048];
    snprintf(src, sizeof(src), "%s/test_charsign_%d.c", td, pid);
    snprintf(bin, sizeof(bin), "%s/test_charsign_%d.bin", td, pid);

    FILE *sf = fopen(src, "w");
    if (!sf) return -1;
    fputs("int main(void){char c=(char)0xff; return (int)c;}\n", sf);
    fclose(sf);

    snprintf(cmd, sizeof(cmd), "%s %s %s -o %s", rcc, flags, src, bin);
    int rc = system(cmd);
    remove(src);
    if (rc != 0) {
        remove(bin);
        return -1;
    }
    snprintf(cmd, sizeof(cmd), "%s", bin);
    int status = system(cmd);
    remove(bin);
#ifdef _WIN32
    *out_val = status;
#else
    *out_val = (status >= 0) ? (status >> 8) & 0xff : -1;
#endif
    return 0;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int v;

    /* -funsigned-char: (char)0xff must read back as 255. */
    if (compile_and_run(rcc, td, pid, "-funsigned-char", &v) != 0) {
        printf("FAIL: -funsigned-char failed to compile\n");
        return 1;
    }
    if (v != 255) {
        printf("FAIL: -funsigned-char: expected 255, got %d\n", v);
        return 2;
    }

    /* -fsigned-char: (char)0xff must read back as 255 mod 256 sign-extended,
     * i.e. exit code 255 either way on POSIX (8-bit wait status) -- so
     * verify via a value that differs when sign-extended through `int`
     * before truncation back to the 8-bit exit status. */
    {
        char src[600], bin[600], out[600], cmd[2048];
        snprintf(src, sizeof(src), "%s/test_charsign2_%d.c", td, pid);
        snprintf(bin, sizeof(bin), "%s/test_charsign2_%d.bin", td, pid);
        snprintf(out, sizeof(out), "%s/test_charsign2_%d.out", td, pid);
        FILE *sf = fopen(src, "w");
        if (!sf) { printf("FAIL: cannot write source\n"); return 3; }
        fputs("#include <stdio.h>\n"
              "int main(void){char c=(char)0xff; int i=c; printf(\"%d\\n\", i); return 0;}\n",
              sf);
        fclose(sf);
        snprintf(cmd, sizeof(cmd), "%s -fsigned-char %s -o %s", rcc, src, bin);
        int rc = system(cmd);
        remove(src);
        if (rc != 0) { printf("FAIL: -fsigned-char failed to compile\n"); return 4; }
        snprintf(cmd, sizeof(cmd), "%s > %s", bin, out);
        system(cmd);
        remove(bin);
        char buf[64] = {0};
        FILE *of = fopen(out, "r");
        if (of) { if (fgets(buf, sizeof(buf), of)) {} fclose(of); }
        remove(out);
        int iv = atoi(buf);
        if (iv != -1) {
            printf("FAIL: -fsigned-char: expected -1, got %d\n", iv);
            return 5;
        }
    }

    /* -fwrapv must be accepted, not rejected. */
    if (compile_and_run(rcc, td, pid, "-fwrapv", &v) != 0) {
        printf("FAIL: -fwrapv rejected\n");
        return 6;
    }

    /* --pedantic-errors (GNU double-dash long form) must be accepted. */
    if (compile_and_run(rcc, td, pid, "--pedantic-errors", &v) != 0) {
        printf("FAIL: --pedantic-errors rejected\n");
        return 7;
    }

    /* The real-world combination that broke test_camgunz_cmp: bare
     * -Werror alongside -funsigned-char/-fwrapv/--pedantic-errors must
     * not hard-error on these now-recognized flags. */
    if (compile_and_run(rcc, td, pid,
                         "-Werror -funsigned-char -fwrapv --pedantic-errors",
                         &v) != 0) {
        printf("FAIL: -Werror + funsigned-char/fwrapv/pedantic-errors combo rejected\n");
        return 8;
    }

    printf("OK\n");
    return 0;
}
