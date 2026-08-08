/* codegen.c's gen_addr() for ND_ASSIGN (an assignment expression used as
 * an lvalue -- e.g. (a = b).member, &(a = b), or a chain assignment whose
 * RHS is itself an assignment) computed the address of the assignment's
 * *target* without ever emitting the assignment's store: `case ND_ASSIGN:
 * return gen_addr(node->lhs);` silently dropped the side effect, leaving
 * the target unmodified/uninitialized while the caller read through its
 * address as if the store had already happened.
 *
 * This is exactly the shape mruby's word-boxed mrb_value representation
 * uses pervasively: GCC-style macros like
 *   #define mrb_hash_p(o) (!mrb_immediate_p(o) && mrb_val_union(o).bp->tt == ...)
 * expand their parameter `o` multiple times, so a call site like
 * `mrb_hash_p(kdict = regs[kidx])` re-expands the assignment-as-argument
 * textually several times -- each occurrence needs the assignment's real
 * side effect visible, not just its target's (uninitialized) address.
 * Because this bug silently read garbage instead of raising a compile
 * error, it manifested as wrong values deep in mruby's VM (spurious
 * "missing keyword" ArgumentErrors, wrong Hash#key? results) rather than
 * a build failure -- found via mruby 4.0.0's mrbtest suite: 5 assertions
 * crashed with bogus ArgumentErrors before this fix (Crash: 5), 0 after.
 *
 * Fixed by generating the assignment for real (gen(node), which performs
 * the store) before computing/using the target's address: for
 * struct/union/array/complex targets gen() already returns the
 * destination address directly (reused as-is); for scalar targets gen()
 * returns the assigned *value*, discarded before re-taking the address.
 */
#include <assert.h>
#include <stdint.h>

typedef struct {
    uintptr_t w;
} boxed;

int main(void) {
    boxed regs[4];
    regs[1].w = 42;

    /* Member access on the *result* of a struct assignment expression:
     * the assignment must actually happen before .w is read. */
    boxed kdict;
    uintptr_t r0 = (kdict = regs[1]).w;
    assert(r0 == 42);
    assert(kdict.w == 42);

    /* Same assignment-expression-as-lvalue evaluated twice within one
     * expression -- the exact shape a GCC-style function-like macro
     * produces when its parameter appears more than once in the body
     * (e.g. WORDBOX_OBJ_TYPE_P's `!mrb_immediate_p(o) && ...(o)...`):
     * `&&`/`||` sequence their operands, so both reads must see the real
     * stored value, not garbage from a skipped store. (`+` would leave
     * the two assignments unsequenced -- avoided here on purpose.) */
    boxed kdict2;
    int both_ok = (kdict2 = regs[1]).w == 42 && (kdict2 = regs[1]).w == 42;
    assert(both_ok);
    assert(kdict2.w == 42);

    /* Chain assignment through a struct type (d = e = a): the inner
     * assignment must run before the outer one copies from it. */
    boxed d, e;
    boxed a = {99};
    d = e = a;
    assert(d.w == 99);
    assert(e.w == 99);

    return 0;
}
