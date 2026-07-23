/* GCC/Clang treat "strlen(STR_LITERAL)" (and its __builtin_strlen spelling,
 * which the kernel's own headers #define to plain "strlen" for non-checker
 * builds — see src/preprocess.c's define_pre("__builtin_strlen", "strlen"))
 * as a constant-foldable call when the argument is a string literal, the
 * same way sizeof(literal) is — a widely-relied-on extension for kernel
 * code like:
 *   _Static_assert(sizeof(lit) - 1 == strlen(lit), "no embedded NUL byte");
 *
 * Two related bugs, both in eval_const_expr()'s ND_FUNCALL case:
 *
 * 1. There was no folding for strlen()/__builtin_strlen() calls at all —
 *    only certain bit-counting builtins (clz/ctz/popcount/bswap) were
 *    special-cased, so this always failed with "static_assert condition
 *    must be constant" even for the simplest case.
 *
 * 2. Once folding was added keyed off node->funcname, it still failed
 *    whenever "strlen" had a real prototype already in scope (e.g. via
 *    <linux/string.h>): the parser resolves such a call through
 *    node->lhs (a reference to the declared function) instead of
 *    populating node->funcname (which is only set for a still-undeclared
 *    "plain identifier" call) — so the funcname-only check silently
 *    stayed unfolded, again "condition must be constant".
 *
 * Found via a real Linux kernel build: kernel/workqueue.c's very first
 * module_param_named() call expands (through moduleparam.h's
 * __MODULE_PARM_TYPE()/MODULE_INFO()) to exactly this pattern with the
 * parameter's "name:type" string — and by that point in the translation
 * unit, <linux/string.h> has long since declared "strlen" for real. The
 * same construct also blocked mm/slab_common.c the same way.
 */
extern unsigned long strlen(const char *s); /* matches <linux/string.h> */

_Static_assert(sizeof("hello") - 1 == __builtin_strlen("hello"),
                "embedded NUL byte");
_Static_assert(sizeof("cpu_intensive_thresh_us:ulong") - 1 ==
                strlen("cpu_intensive_thresh_us:ulong"),
                "embedded NUL byte");

int main(void)
{
    return 0;
}
