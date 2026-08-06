/* Regression test: call under extreme register pressure.
 *
 * With all 8 x86-64 allocatable VRegs occupied by live values of a
 * right-deep expression, a nested function call triggers use_staging.
 * The alloc_reg() spill of an outer VReg while evaluating the call's
 * argument must preserve the spilled value across the call so the
 * binary-op same-register combining path can recover it after the
 * call returns.
 *
 * Without the fix (staging cleared spilled_regs and the marshal
 * materialize_reg reloaded a stale spill-slot value over the
 * staging-loaded arg), the right-deep sum returned 129 instead of
 * 100 — the 5th term (v+5) contributed 40 instead of 8.  The fix:
 * staging keeps spilled_regs for recycled outer VRegs, and the
 * marshal skips materialize_reg for staging-loaded registers.
 */

#include <stdio.h>
#include <stdlib.h>

static int failures;

#define assert_eq(a, b, msg) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL: %s: expected %lld, got %lld\n", msg, _b, _a); \
        failures++; \
    } \
} while (0)

static long id(long x) { return x; }

int main(void) {
    /* Win64: the #ifdef _WIN32 marshal path lacks staging reload,
     * so the staging spill-preservation fix doesn't apply there. */
#ifndef _WIN32
    volatile long v = 3;
    /* Right-deep: (v+1) + (v+2 + (... + (v+8 + id(40))...)).
     * 8 live VRegs + call with 1 GP arg → use_staging=1, free_reg_count=0. */
    long r = (v + 1) + ((v + 2) + ((v + 3) + ((v + 4) +
             ((v + 5) + ((v + 6) + ((v + 7) + ((v + 8) +
             id(40))))))));
    /* r should equal v+1..v+8 + 40 = 4+5+6+7+8+9+10+11+40 = 100 */
    assert_eq(r, 100, "call pressure: right-deep sum with call");
#endif

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL CALL PRESSURE TESTS PASSED\n");
    return failures ? 1 : 0;
}
