/* __has_builtin(NAME): a clang/GCC preprocessor extension used pervasively
 * by portable feature-detection code (autoconf/meson/muon's "does the
 * compiler provide __builtin_X" probes) to ask "does this compiler
 * recognize this __builtin_* name" without triggering a hard error for
 * compilers/names that don't. rcc previously didn't implement it at all
 * (#ifdef __has_builtin was always false), which broke any such probe
 * that used the now-idiomatic
 *   #ifdef __has_builtin
 *   #if __has_builtin(__builtin_foo)
 *   ...
 * fallback pattern — the #ifdef branch silently took the "not available"
 * path even when rcc genuinely implements __builtin_foo.
 *
 * Like __has_include/__has_c_attribute, __has_builtin can be used in the
 * expression of #if/#elif and is treated as a defined macro by
 * #ifdef/#ifndef/defined, but nowhere else.
 */

#ifndef __has_builtin
#error "__has_builtin not defined"
#endif

#ifdef __has_builtin
/* OK */
#else
#error "#ifdef __has_builtin should be true"
#endif

#if !defined(__has_builtin)
#error "defined(__has_builtin) should be true"
#endif

/* A handful of builtins rcc genuinely implements (across parser.c's direct
 * dispatch, codegen.c's bi_s_* table, and preprocess.c's __builtin_X ->
 * library-name macro aliases) must report true. */
#if !__has_builtin(__builtin_alloca)
#error "__has_builtin(__builtin_alloca) should be true"
#endif
#if !__has_builtin(__builtin_memcpy)
#error "__has_builtin(__builtin_memcpy) should be true"
#endif
#if !__has_builtin(__builtin_expect)
#error "__has_builtin(__builtin_expect) should be true"
#endif
#if !__has_builtin(__builtin_popcount)
#error "__has_builtin(__builtin_popcount) should be true"
#endif
#if !__has_builtin(__builtin_types_compatible_p)
#error "__has_builtin(__builtin_types_compatible_p) should be true"
#endif
#if !__has_builtin(__builtin_va_start)
#error "__has_builtin(__builtin_va_start) should be true"
#endif

/* An unrecognized name must report false, not error out. */
#if __has_builtin(__builtin_totally_fake_name_xyz123)
#error "__has_builtin(__builtin_totally_fake_name_xyz123) should be false"
#endif

/* The muon/meson "has function alloca" probe's own __has_builtin
 * fallback shape: some of alloca()'s callers #define it away first to
 * see whether a REAL, distinctly-named library symbol exists (a separate,
 * unrelated check); when that fails, this __has_builtin-guarded fallback
 * is the one that must positively confirm the compiler builtin exists,
 * without ever reaching the __builtin_alloca; statement below (which would
 * be an "undeclared variable" error on a compiler that doesn't special-case
 * bare uses of a builtin name as a value). This exact shape previously
 * miscompiled to a real error on rcc because __has_builtin wasn't defined,
 * so the #elif branch ran instead and referenced the plain identifier
 * "alloca" as if it were an ordinary (never declared) variable. */
static int probe(void) {
#if !1 && !defined(alloca) && !0
#error "No definition for __builtin_alloca found in the prefix"
#endif
#ifdef __has_builtin
#if !__has_builtin(__builtin_alloca)
#error "__builtin_alloca not found"
#endif
#elif !defined(alloca)
    __builtin_alloca;
#endif
    return 0;
}

/* __has_builtin(__builtin_alloca) reporting true must also mean the
 * builtin genuinely works at runtime, not just that the name is known. */
static int use_real_alloca(void) {
#if __has_builtin(__builtin_alloca)
    char *p = (char *)__builtin_alloca(32);
    for (int i = 0; i < 32; i++) p[i] = 'z';
    for (int i = 0; i < 32; i++)
        if (p[i] != 'z') return 1;
    return 0;
#else
    return 1; /* unreachable given the #if checks above already errored */
#endif
}

/* GCC PR92261 "syntax errors on __has_builtin (__has_builtin)": unlike
 * real GCC (which only supports __has_builtin inside #if/#elif), Clang
 * treats it as a general preprocessor construct usable in ordinary
 * expression context too. rcc's #if-context handling deferred the outer
 * `__has_builtin` token unevaluated with nothing downstream able to
 * parse it, AND separately let the raw argument token fall through to
 * plain object-macro expansion (`#define __has_builtin 1`) before
 * anyone checked it -- corrupting the self-referential case in
 * particular. Verified against real clang to produce identical values:
 * __has_builtin(__has_builtin) is 0 (the operator itself isn't a
 * builtin function); __has_builtin(__builtin_memcpy) is 1. */
static int test_outside_if(void) {
    int self = __has_builtin(__has_builtin);
    int direct = __has_builtin(__builtin_memcpy);
    int fake = __has_builtin(__builtin_totally_fake_name_xyz123);
    if (self != 0) return 1;
    if (direct != 1) return 2;
    if (fake != 0) return 3;
    return 0;
}

int main(void) {
    if (probe() != 0) return 2;
    if (use_real_alloca() != 0) return 3;
    if (test_outside_if() != 0) return 4;
    return 0;
}
