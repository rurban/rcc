/* Regression test: `A && CONST_FALSE` / `A || CONST_TRUE` -- where the
 * constant operand is on the RHS and A is a genuinely non-constant
 * runtime expression -- failed to eliminate a call living in the
 * corresponding dead `if` branch, breaking the link against an
 * intentionally-undefined "impossible case" function.
 *
 * `optimize_node()` (opt.c) already folded the mirror-image shape
 * (`CONST && f()`/`CONST || f()`, constant on the LHS) by dropping the
 * whole node down to a bare 0/1 literal -- safe because eval_const_expr()
 * only ever succeeds through side-effect-free constructs, so the LHS's
 * own evaluation was never lost. But `libtommath`'s own `MP_HAS(x)`
 * feature-detection macro (`sizeof(STRINGIZE(x##_C)) == 1u`, a compile-
 * time-constant `sizeof`-of-a-string-literal comparison) puts the
 * constant on the RHS instead: `if ((err != OK) && MP_HAS(FEATURE)) err =
 * feature_fn(...);`, guarding a call to a function ifdef'd out (declared
 * but never DEFINED) on this platform. Since only the LHS was checked,
 * the untaken `if` branch was never eliminated: codegen emitted the call
 * as ordinary, unreachable-at-runtime straight-line code that still
 * carried a real relocation against the never-defined symbol --
 * "undefined reference" at link time even though the branch could never
 * actually execute.
 *
 * Two-part fix:
 * 1. A new RHS-side counterpart of the existing LHS fold: when eval_-
 *    const_expr() on the RHS alone determines the &&/|| result (RHS==0
 *    for &&, RHS!=0 for ||), replace the whole node with `ND_COMMA(lhs,
 *    const)` -- the LHS (which C requires to always run) still executes
 *    for its own side effects/ordering, but the provably side-effect-
 *    free RHS (and anything -- including a call -- it might otherwise
 *    have gated) disappears entirely.
 * 2. The `if`-with-constant-condition dead-branch eliminator now also
 *    recognizes exactly this `(prefix, CONST)` comma shape (not just a
 *    directly-constant condition): it structurally drops the untaken
 *    branch -- and any call inside it -- while still running `prefix`
 *    first. Without this second half, the ND_COMMA transform alone only
 *    fixes the VALUE (a real, always-false runtime compare), not the
 *    branch's CODE -- the dead call would still be emitted (and require
 *    linking) as unreachable-but-present code.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[256], exef[256], cmd[768];
    snprintf(srcf, sizeof(srcf), "%s/test_logand_rhs_dce_%d.c", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_logand_rhs_dce_%d", td, pid);

    // Mirrors libtommath's s_mp_rand_platform.c exactly: MP_HAS(x) is a
    // sizeof-of-a-stringized-macro compile-time constant; NEVER_DEFINED_C
    // is (deliberately) never #define'd, so MP_HAS(NEVER_DEFINED) is a
    // compile-time-constant 0 living on the RHS of `&&`. undefined_func()
    // is declared but never defined anywhere in this TU -- if the dead
    // branch calling it is not fully eliminated, the link fails.
    static const char src[] =
        "#include <stddef.h>\n"
        "extern int undefined_func(void);\n"
        "extern int undefined_func2(void);\n"
        "#define STRINGIZE_(x) #x\n"
        "#define STRINGIZE(x) STRINGIZE_(x)\n"
        "#define MP_HAS(x) (sizeof(STRINGIZE(x##_C)) == 1u)\n"
        "int main(void) {\n"
        "    int err = 1;\n"
        "    /* && direction (the real libtommath bug): RHS-constant-\n"
        "     * false (MP_HAS(NEVER_DEFINED) == 0) makes the whole\n"
        "     * condition always false regardless of the runtime LHS,\n"
        "     * so this call must never be linked in. */\n"
        "    if ((err != 0) && MP_HAS(NEVER_DEFINED)) err = undefined_func();\n"
        "    /* || mirror: RHS-constant-TRUE makes the whole condition\n"
        "     * always true regardless of the runtime LHS, so the ELSE\n"
        "     * branch's call must never be linked in. */\n"
        "    if ((err != 999) || !MP_HAS(NEVER_DEFINED)) err = 0; else err = undefined_func2();\n"
        "    return err;\n"
        "}\n";
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    // opt.c's optimize() pass (where this fold lives) only runs at
    // -O1 and above (main.c: `if (opt_O1 || opt_finline || opt_funroll)`)
    // -- matching real libtommath's own build, which always compiles at
    // -O3, and the pre-existing sibling (LHS-constant) fold this mirrors.
    // Must actually LINK (not just `-c` compile to an object file) --
    // the bug only surfaces when the linker tries to resolve the dead
    // branch's still-present call to the never-defined symbol.
    snprintf(cmd, sizeof(cmd), "%s -O1 -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);

    if (rc != 0) {
        printf("FAIL: -O1 compile+link failed (rc=%d) -- RHS-constant "
               "&&/|| branch elimination regressed\n", rc);
        return 2;
    }

    // Also confirm it actually runs and returns 0 (err ends at 0 via the
    // || branch, both dead calls correctly never taken).
    rc = system(exef);
    remove(exef);
    if (rc != 0) {
        printf("FAIL: compiled binary exited %d, want 0\n", rc);
        return 3;
    }
    printf("OK\n");
    return 0;
}
