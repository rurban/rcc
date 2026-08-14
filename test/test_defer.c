/* C23 `defer` (WG14 N3199 / TS 25755, matching clang's experimental
 * `-fdefer-ts` flag name): `defer statement;` registers `statement` to
 * run, LIFO with every other pending cleanup/defer, when the enclosing
 * scope exits (fall-through, return, break, continue, or goto out of
 * it) -- found via nob.h (https://github.com/tsoding/nob.h), which uses
 * it throughout its file-I/O helpers (nob_write_entire_file,
 * nob_read_entire_dir, nob_copy_directory_recursively, ...).
 *
 * `defer { ... }` is recognized unconditionally: an ordinary identifier
 * can never be followed directly by `{` in valid C (not a declaration
 * -- `defer` is never a typedef name -- and not a continuable
 * expression), so there is no real-world program this could misparse.
 * `defer <stmt>;` without braces (nob.h's own preferred style, e.g.
 * `defer if (f) fclose(f);`) is genuinely ambiguous with a call to a
 * function literally named `defer`, so it stays gated behind
 * `-fdefer-ts` -- tested via a subprocess below, since this file itself
 * must compile without the flag.
 *
 * Implementation: a `defer` statement is a zero-storage marker
 * synthesized directly onto the same `locals` chain
 * __attribute__((cleanup(...))) variables use (parser.c), so it
 * inherits that mechanism's existing LIFO/return/fall-through
 * threading. Three real bugs were found and fixed while wiring this up
 * (each reproduced below with its own dedicated case, since any one
 * regressing independently would otherwise go unnoticed):
 *
 * 1. A pending defer/cleanup runs arbitrary code between materializing
 *    a `return` expression's value and the actual jump to the
 *    function's epilogue -- codegen placed the return value in the ABI
 *    return register(s) *before* running cleanup, so a defer body that
 *    itself makes a call (e.g. fclose()) silently clobbered the
 *    already-computed return value. Fixed by spilling the return
 *    register(s) to a scratch stack slot before cleanup and reloading
 *    after, whenever a return has anything pending.
 * 2. codegen.c's own shared function epilogue (every `return`
 *    converges on one `.L.return.<fn>` label) is where an ordinary
 *    top-level __attribute__((cleanup)) variable's teardown call
 *    already fires exactly once, regardless of how many `return`
 *    statements a function has -- parser.c advances a bookkeeping
 *    pointer (current_fn_scope_locals) past *every* new top-level
 *    declaration specifically so each individual `return`'s own
 *    cleanup range excludes anything the shared epilogue already owns.
 *    A `defer` marker must equally participate in that advance;
 *    without it, a *later* top-level declaration (e.g. a second local
 *    declared after the `defer`) silently excludes the marker from
 *    every subsequent return's own range while the shared epilogue
 *    also skips it (since it now looks like a nested-scope-only
 *    entry), and the defer body never runs at all -- found via
 *    test_nob's own cmd_args_passing.c: nob_write_entire_file()
 *    declares `FILE *f`, `defer if (f) fclose(f);`, then loops writing
 *    with `while (size > 0) { size_t n = fwrite(...); ...; }` before
 *    its final `return true;` -- the loop's own local `n` was exactly
 *    such a later top-level declaration, so the file was never
 *    actually flushed/closed before the caller went on to compile it.
 * 3. A function called *only* from inside a `defer` body is invisible
 *    to the dead-code-elimination pass (opt.c's
 *    eliminate_unused_static_inline): a defer's statement Node lives
 *    solely in LVar.defer_stmt, reached only via codegen's own
 *    dedicated gen(var->defer_stmt) call at each scope-exit site, never
 *    as a child of the enclosing function's ordinary body Node tree
 *    that the DCE reachability scan walks. At -O1 and above, such a
 *    callee was wrongly treated as unreferenced and omitted, leaving a
 *    real call site with no definition to link against -- fixed by
 *    also scanning every live function's LVar chain for a defer_stmt.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int order[8];
static int order_n;

static int test_lifo_order(void)
{
    order_n = 0;
    defer { order[order_n++] = 1; }
    defer { order[order_n++] = 2; }
    defer { order[order_n++] = 3; }
    /* fall through: must fire 3, 2, 1 (reverse declaration order) */
    return 0;
}

static int early_return_fired;

/* A defer registered before an early return must still fire -- checked
 * via a global the *caller* reads after the call returns, since the
 * defer body only runs during the return itself (never observable from
 * inside the function before that point). */
static int test_early_return_fires_defer(int take_early_path)
{
    early_return_fired = 0;
    defer { early_return_fired = 1; }
    if (take_early_path) return 1;
    return 0;
}

/* Bug 1 & 3 repro: a defer body that calls another (otherwise
 * unreferenced) function must not clobber an already-computed scalar
 * return value, and that callee must survive DCE at every -O level. */
static int stomp_return_reg(int *unused)
{
    (void)unused;
    return 0xdead; /* would land in the same ABI register as the caller's
                     * own about-to-return value if not protected */
}

static int test_return_value_survives_defer_call(void)
{
    int guard = 0;
    defer { stomp_return_reg(&guard); }
    return 7;
}

/* Bug 2 & 3 repro: a *second* top-level declaration after the defer,
 * plus a loop, must not prevent the defer from firing on the final
 * fall-through return -- this is exactly nob_write_entire_file()'s own
 * shape. mark() is also unreferenced outside this one defer body,
 * exercising the same DCE fix as above. */
static int calls_after_second_decl;
static void mark(int *p) { (void)p; calls_after_second_decl++; }

static int test_defer_survives_later_decl_and_loop(int n)
{
    int guard = 0;
    defer { mark(&guard); }
    int i = 0; /* the "later top-level declaration" that used to break it */
    while (i < n) i++;
    return i;
}

/* The exact nob_write_entire_file() shape: defer registered, a write
 * loop runs, then an unconditional `return true;` at the very end --
 * the file must be fully flushed/closed by the time this returns. */
static int write_it(const char *path, const char *data, size_t size)
{
    int result = 1;
    FILE *f = fopen(path, "wb");
    defer { if (f) fclose(f); }
    if (f == NULL) return 0;
    const char *buf = data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (n == 0) return 0;
        size -= n;
        buf += n;
    }
    return result;
}

static int test_write_flushed_before_return(void)
{
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char path[256];
    snprintf(path, sizeof(path), "%s/test_defer_write_%d.txt", td, pid);
    static const char msg[] = "hello from test_defer\n";

    if (!write_it(path, msg, strlen(msg))) return 1;

    /* Reopen immediately (no process exit in between, so no CRT-exit
     * flush can be masking a real bug) and read the content back. */
    FILE *check = fopen(path, "rb");
    if (!check) { remove(path); return 2; }
    char buf[128] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, check);
    fclose(check);
    remove(path);
    if (n != strlen(msg) || strcmp(buf, msg) != 0) return 3;
    return 0;
}

/* An empty defer body and a defer whose body never runs (never reached
 * due to an earlier return) must both be harmless no-ops. */
static int side_effect;
static int test_empty_and_unreached_defer(int take_early_path)
{
    defer { }
    if (take_early_path) {
        defer { side_effect = 99; }
        return 0; /* the second defer above fires; the empty one is a no-op */
    }
    return 0;
}

int main(void)
{
    int rc;

    order_n = 0;
    if ((rc = test_lifo_order())) return 10 + rc;
    if (order_n != 3 || order[0] != 3 || order[1] != 2 || order[2] != 1) {
        printf("FAIL: lifo order = [%d %d %d] (n=%d), want [3 2 1]\n",
               order[0], order[1], order[2], order_n);
        return 1;
    }

    if (test_early_return_fires_defer(1) != 1 || !early_return_fired) {
        printf("FAIL: defer did not fire on the early-return path "
               "(fired=%d)\n",
               early_return_fired);
        return 2;
    }
    if (test_early_return_fires_defer(0) != 0 || !early_return_fired) {
        printf("FAIL: defer did not fire on the fall-through path "
               "(fired=%d)\n",
               early_return_fired);
        return 2;
    }

    if (test_return_value_survives_defer_call() != 7) {
        printf("FAIL: return value clobbered by a defer body's own call\n");
        return 3;
    }

    calls_after_second_decl = 0;
    int loopr = test_defer_survives_later_decl_and_loop(3);
    if (loopr != 3 || calls_after_second_decl != 1) {
        printf("FAIL: defer after a later decl + loop: result=%d calls=%d "
               "(want 3, 1)\n",
               loopr, calls_after_second_decl);
        return 4;
    }

    if ((rc = test_write_flushed_before_return())) {
        printf("FAIL: file not flushed/closed before write_it() returned "
                "(case %d)\n",
               rc);
        return 5;
    }

    side_effect = 0;
    if (test_empty_and_unreached_defer(1) != 0 || side_effect != 99) {
        printf("FAIL: nested/empty defer mishandled (side_effect=%d)\n", side_effect);
        return 6;
    }

    /* -fdefer-ts gates the brace-less single-statement form (genuinely
     * ambiguous with a call to a function literally named `defer`); the
     * `defer { ... }` block form above needs no flag at all. Verified
     * via a subprocess since this translation unit itself must compile
     * cleanly without the flag. */
    {
        const char *rcc = find_rcc();
        const char *td = get_tmpdir();
        int pid = (int)getpid();
        char srcf[256], objf[256], cmd[768];
        snprintf(srcf, sizeof(srcf), "%s/test_defer_ts_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_defer_ts_%d.o", td, pid);

        static const char src[] =
            "#include <stdio.h>\n"
            "int f(FILE *fp) {\n"
            "    defer if (fp) fclose(fp);\n"
            "    return 1;\n"
            "}\n"
            "int main(void) { return 0; }\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 7; }
        fputs(src, f);
        fclose(f);

        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int wrc = system(cmd);
        remove(objf);
        if (wrc == 0) {
            printf("FAIL: brace-less 'defer <stmt>;' must require -fdefer-ts\n");
            remove(srcf);
            return 8;
        }

        snprintf(cmd, sizeof(cmd), "%s -fdefer-ts -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        wrc = system(cmd);
        remove(objf);
        remove(srcf);
        if (wrc != 0) {
            printf("FAIL: brace-less 'defer <stmt>;' should compile with -fdefer-ts\n");
            return 9;
        }
    }

    /* `return` inside a defer body is ill-formed (WG14 N3199: a defer
     * body executes during scope unwind and cannot itself return a
     * value from the enclosing function) -- must be a clean diagnostic,
     * not silently accepted or a crash. */
    {
        const char *rcc = find_rcc();
        const char *td = get_tmpdir();
        int pid = (int)getpid();
        char srcf[256], objf[256], cmd[768];
        snprintf(srcf, sizeof(srcf), "%s/test_defer_ret_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_defer_ret_%d.o", td, pid);

        static const char src[] =
            "int f(void) {\n"
            "    defer { return 1; }\n"
            "    return 0;\n"
            "}\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 11; }
        fputs(src, f);
        fclose(f);

        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int wrc = system(cmd);
        remove(objf);
        remove(srcf);
        if (wrc == 0) {
            printf("FAIL: 'return' inside a defer body must be a compile error\n");
            return 12;
        }
    }

    printf("OK\n");
    return 0;
}
