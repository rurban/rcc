// Unit test for the -O2 SSA peephole's new bitwise / arithmetic identities:
//   x & ~x -> 0,  x | ~x -> ~0,  x ^ ~x -> ~0
//   x & (x | c) -> x,  x | (x & c) -> x
//   x & c1 & c2 -> x & (c1 & c2),  x | c1 | c2,  x ^ c1 ^ c2
//   x - (x + c) -> -c,  x - (x - c) -> c
//   x / 2^k -> x * 2^-k (float),  x / -1.0 -> -x (float)
//
// The SSA pass (and these peepholes) only runs at -O2+, while the unit-test
// harness compiles this file at -O1 by default (see rccflags in run_tests.c).
// So, like test_inline.c, drive rcc as a subprocess with an explicit -O2:
// compile a program that applies each identity against a *runtime* value (a
// parameter, so it is not constant-folded away), run it, and require it to
// pass its own checks. The identities hold at any optimization level, so this
// is a regression guard: if an optimization rewrites a pattern to the wrong
// value, the -O2 build's checks fail and the test reports an error.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static void check_ssa_bitwise(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[160], exef[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ssa_bitwise_%d.c", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_ssa_bitwise_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif

    static const char src[] =
        "static int fails = 0;\n"
        "static void ck(int c) { if (!c) fails++; }\n"
        "static int t_and_notx(int x) { return x & ~x; }\n"
        "static int t_or_notx(int x) { return x | ~x; }\n"
        "static int t_xor_notx(int x) { return x ^ ~x; }\n"
        "static int t_and_abs(int x) { return x & (x | 0x55); }\n"
        "static int t_or_abs(int x) { return x | (x & 0xAA); }\n"
        "static int t_and_comb(int x) { return x & 0xFF & 0x0F; }\n"
        "static int t_or_comb(int x) { return x | 0x0F | 0x30; }\n"
        "static int t_xor_comb(int x) { return x ^ 0x0F ^ 0x30; }\n"
        "static int t_sub_add(int x) { return x - (x + 7); }\n"
        "static int t_sub_sub(int x) { return x - (x - 5); }\n"
        "static double t_div_pow2(double x) { return x / 8.0; }\n"
        "static double t_div_pow2b(double x) { return x / 64.0; }\n"
        "static double t_div_neg1(double x) { return x / -1.0; }\n"
        "int main(void) {\n"
        "  int xs[6] = {0, 1, -1, 0x7FFFFFFF, 0x5A5A, 123456};\n"
        "  for (int i = 0; i < 6; i++) {\n"
        "    int x = xs[i];\n"
        "    ck(t_and_notx(x) == 0);\n"
        "    ck(t_or_notx(x) == -1);   /* ~0 */\n"
        "    ck(t_xor_notx(x) == -1);  /* ~0 */\n"
        "    ck(t_and_abs(x) == x);\n"
        "    ck(t_or_abs(x) == x);\n"
        "    ck(t_and_comb(x) == (x & 0x0F));\n"
        "    ck(t_or_comb(x) == (x | 0x3F));\n"
        "    ck(t_xor_comb(x) == (x ^ 0x3F));\n"
        "    ck(t_sub_add(x) == -7);\n"
        "    ck(t_sub_sub(x) == 5);\n"
        "  }\n"
        "  double dxs[4] = {0.0, 1.0, -3.75, 1e10};\n"
        "  for (int i = 0; i < 4; i++) {\n"
        "    double x = dxs[i];\n"
        "    ck(t_div_pow2(x) == x * 0.125);\n"
        "    ck(t_div_pow2b(x) == x * 0.015625);\n"
        "    ck(t_div_neg1(x) == -x);\n"
        "  }\n"
        "  return fails == 0 ? 0 : 1;\n"
        "}\n";

    FILE *f = fopen(srcf, "w");
    assert(f && "cannot write temp source for SSA bitwise-identity check");
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -O2 -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    assert(rc == 0 && "-O2 compile of SSA bitwise-identity program failed");

    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    rc = system(cmd);
    remove(exef);
    remove(srcf);
    assert(rc == 0 && "SSA bitwise-identity program produced wrong results at -O2");
}

int main(void) {
    check_ssa_bitwise();
    return 0;
}
