/* Two soundness bugs in -O2's tail-call optimization
 * (codegen.c's ND_RETURN tail-jmp fast path, `opt_finline && opt_funroll`):
 *
 * 1. Frame-escape aliasing: a tail-jmp deallocates the current frame
 *    BEFORE the callee's own prologue reuses that exact stack region.
 *    Unsound whenever the callee can still read through a pointer into
 *    it -- e.g. GCC torture pr36339.c: a local array's address (via
 *    plain array-to-pointer decay, no explicit `&`) is passed to a
 *    tail-called function, which then dereferences it after the
 *    caller's frame -- containing the array's actual bytes -- has
 *    already been overwritten by the callee's own prologue. Fixed by
 *    tail_call_frame_may_escape(): any array/struct/union/VLA
 *    parameter or local, or any explicit `&var` of one, disqualifies
 *    the whole function's tail-jmps.
 *
 * 2. Builtin misidentification: `return __builtin_bswap64(x);` matches
 *    every other eligibility check (single scalar arg, scalar return)
 *    and declare_builtin_on_demand() synthesizes a real extern-function
 *    LVar for `__builtin_bswap64` so the "is this callable" check
 *    passes -- but that name has no actual linkable symbol at all; the
 *    ordinary (non-tail) call path recognizes it by name and expands
 *    it inline (cg_builtins.c) instead of ever emitting a real call.
 *    Tail-jmp'ing to the bare name linked as "undefined reference to
 *    `__builtin_bswap64'" (GCC torture bswap-1.c). Fixed by excluding
 *    any `__builtin_*`-named call from the tail-jmp fast path.
 */
#include <stdlib.h>

/* Case 1: local array's address flows into a tail-called function. */
typedef unsigned long uptr;

static int check_heap(uptr tagged_ptr) {
    uptr *hp = (uptr *)(void *)((char *)tagged_ptr - 1);
    if (hp[0] == 42 && hp[1] == 0)
        return 0;
    return -1;
}

static int try_heap(uptr x) {
    uptr heap[2];
    uptr *hp = heap;
    hp[0] = x;
    hp[1] = 0;
    return check_heap((uptr)(void *)((char *)hp + 1));
}

/* Same hazard via an explicit `&` on a scalar local instead of array
 * decay -- must be caught by the explicit-address-taken walk too. */
static int read_back(int *p) {
    return *p == 99 ? 0 : -1;
}

static int try_scalar(int x) {
    int local = x;
    return read_back(&local);
}

/* Case 2: builtins with no real linkable symbol. */
static unsigned long long bswap_it(unsigned long long x) {
    return __builtin_bswap64(x);
}

static unsigned popcount_it(unsigned x) {
    return __builtin_popcount(x);
}

int main(void) {
    if (try_heap(42) != 0) abort();
    if (try_scalar(99) != 0) abort();
    if (bswap_it(0x0102030405060708ULL) != 0x0807060504030201ULL) abort();
    if (popcount_it(0xFFu) != 8) abort();
    return 0;
}
