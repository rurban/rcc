/* A `const T *` function PARAMETER's type qualifier must only apply to
 * that one declaration's parameter type — it must never mutate the
 * shared struct/union Type object of `T` itself, corrupting every other,
 * unrelated (non-const) declaration of `T` elsewhere in the translation
 * unit.
 *
 * declspec() resolves a typedef name to the SAME shared Type* every time
 * it's used (`ty = td->ty`), then — when a leading `const`/`volatile`/
 * `restrict` qualifier is present — is supposed to clone that type before
 * OR'ing the qualifier into ->qual. It called copy_type() for this, but
 * copy_type() *intentionally* returns a struct/union type's own pointer
 * unchanged (so an incomplete forward declaration can still be completed
 * later through every existing reference — see test_align_type_leak.c
 * for the sibling bug in apply_type_align()). Reusing that identity-
 * preserving behavior here meant `const struct S *p` mutated the SHARED
 * struct S Type object's ->qual in place, silently const-qualifying
 * every OTHER declaration of `struct S` in the file too.
 *
 * A const-qualified struct's member reads are eligible for
 * eval_const_expr()'s "read from the static initializer" fast path
 * (real GCC constant-folds reads of const objects the same way) — so a
 * plain, genuinely mutable `struct S g = {0};` whose type got
 * accidentally const-qualified this way had every one of its field
 * reads permanently folded to the *initializer's* value, blind to any
 * later write, anywhere in the file.
 *
 * Found via test_wuffs (test/third_party/TODO.md): wuffs's
 * <smmintrin.h>-adjacent io_buffer helpers declare many `const
 * wuffs_base__io_buffer *buf` parameters; the plain (non-const) global
 * `wuffs_base__io_buffer g_src = {0};` used elsewhere in the same file
 * had its own type silently const-qualified as a side effect, so
 * `if (g_src.meta.wi == g_src.data.len)` — read AFTER g_src.data.len was
 * set to a real runtime value — folded to the STATIC INITIALIZER's
 * "0 == 0" (true) at every -O1+ build, permanently taking a branch that
 * should have depended on runtime state (dropping actual I/O and
 * corrupting decode results with no compiler diagnostic at all).
 */
#include <stddef.h>

typedef struct {
    size_t wi;
    size_t len;
} io_buffer;

/* Any function taking `const io_buffer *` was enough to trip the bug —
 * it need never even be called. */
static size_t reader_length(const io_buffer *buf) {
    return buf->wi - buf->len;
}
/* A second one, to match test_wuffs' shape (several const-param helpers
 * in the same file). */
static size_t reader_extra(const io_buffer *buf) {
    return buf->len;
}

/* Plain, genuinely mutable global — must NOT end up const-qualified. */
io_buffer g_src = {0};

/* Mirrors the real bug: an `if` condition reading two fields of a global
 * whose STATIC INITIALIZER makes both 0, but whose actual runtime value
 * (set below, in a DIFFERENT function) makes them unequal. A buggy
 * const-qualified g_src lets eval_const_expr() fold this to "0 == 0",
 * i.e. always true, regardless of the real values. */
static int check(void) {
    if (g_src.wi == g_src.len) {
        return 1; /* wrongly "equal" if the type-leak bug regresses */
    }
    return 0;
}

int main(void) {
    (void)reader_length;
    (void)reader_extra;
    g_src.wi = 0;
    g_src.len = 65536;
    return check(); /* must be 0: wi(0) != len(65536) at runtime */
}
