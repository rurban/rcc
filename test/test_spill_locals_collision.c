/* Regression test for two stack-layout bugs in codegen.c's x86-64 frame
 * sizing, both root-caused while fixing test_perl's alloc_reg()
 * spill-slot/locals-offset collision (a segfault in Perl_upg_version(),
 * called from S_enable_feature_bundle()'s deeply nested version-bundle
 * ternary chain).
 *
 * Bug 1 (the original test_perl crash): general-purpose spill slots
 * (push_spill_slot()/pop_spill_slot(), used by alloc_reg()'s spill-victim
 * eviction) grew `next_spill_slot` from a small fixed base (24, past the
 * two fixed spill_logand/spill_atomic_old slots) entirely independently of
 * `need` (locals + struct-ret-staging + trampoline-staging + shadow
 * space). A function with enough concurrent spill pressure -- many live
 * temporaries held across several nested calls, as in a long chain of
 * nested ternaries -- could grow the spill region past the point where
 * locals start and silently overwrite a live local's own -N(%rbp) slot.
 * Fixed by re-anchoring the spill region's growth on top of `need`
 * instead of a low, unrelated base.
 *
 * Bug 2 (introduced and caught while fixing bug 1): the __cleanup__
 * epilogue stashes RAX (and RDX, for a function returning a small
 * all-integer struct in the RAX:RDX pair) via spill_offset() *after*
 * Pass 2's body walk -- code Pass 1's dry run never sees, since it only
 * walks fn->body. Re-anchoring general spills to start exactly at `need`
 * (bug 1's fix) put this invisible extra spill_offset(0) call exactly on
 * top of the callee-saved register save area, which also starts at
 * `need` -- corrupting whichever callee-saved register the struct-return
 * path was using to build the result. Fixed by budgeting the cleanup
 * epilogue's known, deterministic extra spill usage (8 bytes for RAX, +8
 * for RDX when applicable) into `need` before placing the callee-saved
 * area.
 */
#include <assert.h>

/* --- Bug 1: heavy concurrent spill pressure must not corrupt locals --- */

struct SV { long long val; };

static long long setnv(struct SV *sv, long long n) { sv->val = n; return n; }
static long long vcmp(struct SV *a, struct SV *b) { return a->val - b->val; }
static struct SV *upg(struct SV *sv) { return sv; }

/* Mirrors S_enable_feature_bundle()'s nested version-bundle ternary: `ver`
 * and `comp_ver` must stay live across every branch's own setnv/vcmp/upg
 * calls, forcing alloc_reg() to spill repeatedly. A pile of named locals
 * declared before the chain and read back after it exposes any spill slot
 * that landed on a local's own offset instead of its own reserved slot.
 *
 * Corruption is reported via *ok (plain arithmetic, no assert()) rather
 * than asserting in-function: assert()'s own call setup interacts badly
 * with this exact heavy-spill shape on the mingw target, a separate,
 * pre-existing issue (reproduces identically pre-fix) that is out of
 * scope here -- this test only defends the locals/spill collision. */
static long long deep_ternary(struct SV *ver, struct SV *comp_ver,
                          long long a, long long b, long long c, long long d,
                          int *ok) {
    long long l0 = a, l1 = b, l2 = c, l3 = d;
    long long l4 = a + 1, l5 = b + 1, l6 = c + 1, l7 = d + 1;
    long long l8 = a + 2, l9 = b + 2, l10 = c + 2, l11 = d + 2;
    long long l12 = a + 3, l13 = b + 3, l14 = c + 3, l15 = d + 3;

    long long r =
      (setnv(comp_ver, 0), vcmp(ver, upg(comp_ver)) >= 0) ? 0 :
      (setnv(comp_ver, 1), vcmp(ver, upg(comp_ver)) >= 0) ? 1 :
      (setnv(comp_ver, 2), vcmp(ver, upg(comp_ver)) >= 0) ? 2 :
      (setnv(comp_ver, 3), vcmp(ver, upg(comp_ver)) >= 0) ? 3 :
      (setnv(comp_ver, 4), vcmp(ver, upg(comp_ver)) >= 0) ? 4 :
      (setnv(comp_ver, 5), vcmp(ver, upg(comp_ver)) >= 0) ? 5 :
      (setnv(comp_ver, 6), vcmp(ver, upg(comp_ver)) >= 0) ? 6 :
      (setnv(comp_ver, 7), vcmp(ver, upg(comp_ver)) >= 0) ? 7 :
      (setnv(comp_ver, 8), vcmp(ver, upg(comp_ver)) >= 0) ? 8 :
      (setnv(comp_ver, 9), vcmp(ver, upg(comp_ver)) >= 0) ? 9 : 10;

    *ok = l0 == a && l1 == b && l2 == c && l3 == d &&
          l4 == a + 1 && l5 == b + 1 && l6 == c + 1 && l7 == d + 1 &&
          l8 == a + 2 && l9 == b + 2 && l10 == c + 2 && l11 == d + 2 &&
          l12 == a + 3 && l13 == b + 3 && l14 == c + 3 && l15 == d + 3;
    return r;
}

/* --- Bug 2: __cleanup__ + a GP-pair (RAX:RDX) struct return --- */

typedef struct { int a; int b; int c; int d; } tsti;

void my_cleanup(tsti *p) {
    p->a = 0x90; p->b = 0x91; p->c = 0x92; p->d = 0x93;
}

static tsti cleanup_struct_return(void) {
    tsti __attribute__((cleanup(my_cleanup))) n;
    n.a = 42; n.b = 43; n.c = 44; n.d = 45;
    return n;
}

int main(void) {
    struct SV ver = {-1};
    struct SV comp_ver = {0};
    int ok = 0;
    long long r = deep_ternary(&ver, &comp_ver, 1, 2, 3, 4, &ok);
    assert(ok); /* locals must survive the heavy-spill chain unmodified */
    assert(r >= 0 && r <= 10); /* exact branch taken is incidental here */

    tsti n = cleanup_struct_return();
    assert(n.a == 42 && n.b == 43 && n.c == 44 && n.d == 45);
    return 0;
}
