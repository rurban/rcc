/* rcc implements __builtin_alloca as a plain preprocessor object macro
 * aliasing to "alloca" (see preprocess.c's define_pre), unlike real
 * GCC/Clang where it's a genuine front-end-recognized identifier immune
 * to ordinary macro substitution rules. glibc's own <alloca.h> defines
 * the *opposite* direction as a function-like macro:
 *   #define alloca(size) __builtin_alloca(size)
 *
 * A translation unit that includes <alloca.h> (directly, or
 * transitively via <stdlib.h> under _GNU_SOURCE, or <stdbit.h> which
 * pulls in <stdlib.h>/<limits.h>) before using __builtin_alloca via
 * another macro (e.g. <string.h>'s GNU strdupa/strndupa) hits a
 * two-macro ping-pong: __builtin_alloca -> alloca -> __builtin_alloca,
 * correctly halted by the standard's hide-set rule (blue-painting
 * __builtin_alloca from the first expansion carries through the
 * second), leaving the *original* "__builtin_alloca(...)" spelling
 * unexpanded in the token stream. Real GCC never hits this because
 * __builtin_alloca isn't a macro there in the first place.
 *
 * codegen's own alloca-recognition previously only matched the literal
 * "alloca" spelling (bi_s_alloca), so an unexpanded "__builtin_alloca"
 * call silently fell through to an ordinary (unresolvable) external
 * call, producing "undefined reference to `__builtin_alloca'" at link
 * time. Found via lwan (any TU pulling in <stdlib.h>/<stdbit.h> before
 * <string.h>'s strdupa is used).
 *
 * Fixed by also interning "__builtin_alloca" (bi_s_builtin_alloca) and
 * normalizing call_target to bi_s_alloca whenever it matches, so every
 * existing alloca-recognition check downstream works unchanged
 * regardless of which spelling the preprocessor's macro ping-pong left
 * behind.
 */
#include <stdlib.h> /* pulls in <alloca.h> under _GNU_SOURCE: #define alloca(n) __builtin_alloca(n) */
#include <stdio.h>
#include <string.h>

/* Hand-expanded GNU strdupa idiom (glibc's own <string.h> strdupa macro
 * body), invoked *after* <stdlib.h>/<alloca.h> is already in scope --
 * the exact ping-pong trigger shape. Written out explicitly (rather than
 * relying on the real strdupa macro) so this test's behavior does not
 * depend on which glibc version -- or its exact feature-test-macro
 * gating -- happens to be installed. */
#define test_strdupa(s)                                                      \
    (__extension__({                                                         \
        const char *__old = (s);                                             \
        size_t __len = strlen(__old) + 1;                                    \
        char *__new = (char *)__builtin_alloca(__len);                       \
        (char *)memcpy(__new, __old, __len);                                 \
    }))

int main(void) {
    char *copy = test_strdupa("hello, rcc");
    if (strcmp(copy, "hello, rcc") != 0) {
        printf("FAIL: strdupa-style __builtin_alloca copy mismatch: %s\n", copy);
        return 1;
    }
    printf("OK\n");
    return 0;
}
