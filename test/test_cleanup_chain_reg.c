/* Regression test: `__attribute__((cleanup(fn)))` where `fn` is a GNU
 * nested function (declared `auto void fn(...)` inside the same
 * function that owns the cleanup-attributed variable, needing access
 * to an outer-scope captured local) crashed at runtime.
 *
 * This is the classic `defer`-via-cleanup-attribute idiom several
 * real-world C2Y-draft `defer` implementations use (e.g. slimcc's own
 * `slimcc.h`: `#define defer ... auto void F(int *); __attribute__
 * ((cleanup(F))) int V; ... inline auto void F(int *V) { BODY }`).
 *
 * `emit_cleanup_var()` (codegen.c) called `emit_direct_call()` for the
 * cleanup function directly, without ever setting up the GNU
 * nested-function static-chain register (%r10 on x86-64 SysV, x18 on
 * ARM64) that an ordinary direct call to a nested function DOES set up
 * (see gen_funcall's `node->lhs->var->is_nested_fn` handling). The
 * nested cleanup function's own prologue still SAVED the chain
 * register (per its normal nested-function ABI), so it silently read
 * garbage -- whatever value happened to be sitting in the chain
 * register at the (unset-up) call site -- as the pointer to its
 * captured outer-scope local, and dereferencing it through that
 * garbage pointer segfaulted the instant the cleanup body touched the
 * captured variable.
 *
 * Found via slimcc (a third-party C compiler with C2Y `defer`
 * support): its own `cc1()` (main.c) uses exactly this shape --
 * `defer { arena_off(&cc1_arena); close_file(out); }` where `out`
 * (a captured `FILE *` local) crashed every single compilation.
 */
#include <assert.h>
#include <stdio.h>

static int cleanup_saw_value;

/* GNU nested-function cleanup handler: reads `*v`, computed from the
 * enclosing function's own captured local `captured` via the
 * static-chain pointer -- the exact mechanism that read garbage. */
static void run_defer_via_cleanup(int captured) {
    auto void handler(int *v);

    __attribute__((cleanup(handler))) int marker;
    (void)marker;

    inline auto void handler(int *v) {
        (void)v;
        cleanup_saw_value = captured;
    }
}

/* Multiple captured variables and a second, unrelated statement
 * before the captured read -- mirrors slimcc's own two-statement
 * cleanup body (`arena_off(&cc1_arena); close_file(out);`) where the
 * FIRST statement's own call clobbers whatever register happened to
 * be holding the (never set up) chain pointer, exactly matching the
 * disassembly that pinpointed this bug. */
static int side_effect_calls;
static void side_effect(void) { side_effect_calls++; }

static int two_stmt_cleanup_result;
static void run_two_statement_cleanup(int captured_a, int captured_b) {
    auto void handler2(int *v);

    __attribute__((cleanup(handler2))) int marker;
    (void)marker;

    inline auto void handler2(int *v) {
        (void)v;
        side_effect();
        two_stmt_cleanup_result = captured_a + captured_b;
    }
}

int main(void) {
    cleanup_saw_value = 0;
    run_defer_via_cleanup(4242);
    assert(cleanup_saw_value == 4242);

    side_effect_calls = 0;
    two_stmt_cleanup_result = 0;
    run_two_statement_cleanup(100, 23);
    assert(side_effect_calls == 1);
    assert(two_stmt_cleanup_result == 123);

    printf("OK\n");
    return 0;
}
