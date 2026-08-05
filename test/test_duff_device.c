/* Duff's-device-style control flow: a `case` label positioned as the body
 * of an `if (0)` statement, used to make code reachable ONLY via a
 * switch's direct jump to the label -- never via ordinary fall-through
 * from the code that precedes the `if`. GCC/Clang idiom:
 *
 *     if (cond)
 *         goto_via_switch_only;
 *         if (0)
 *     case LABEL:
 *         { ...body reachable only by jumping straight to LABEL... }
 *
 * rcc's constant-condition folding for `if (0) { ...labeled body... }`
 * special-cases exactly this shape (to avoid discarding the label as
 * dead code), but generated the labeled body with NO gating at all: a
 * normal, sequential fall-through into the `if(0)`'s source position
 * executed the "dead" body unconditionally, as if the condition were
 * always true, instead of skipping it. A switch's own dispatch (jumping
 * directly to the label, bypassing any code before it) was unaffected,
 * so the bug only manifested via fall-through -- exactly the scenario
 * this construct is meant to guard against.
 *
 * Found via LZ4's frame decompression state machine
 * (lz4frame.c:LZ4F_decompress, `case dstage_getCBlock: ... if (0) case
 * dstage_storeCBlock: {...}`): the dstage_storeCBlock body executed on
 * every single call regardless of dctx->dStage, corrupting the decoder
 * state and making every LZ4F_decompress() call fail with an "unfinished
 * frame" hint instead of completing. */
#include <assert.h>

/* --- test 1: minimal repro, no switch involved -- pure if(0)+case --- */

static int probe_calls;

static int minimal(int state, int enough) {
    int result = -1;
    switch (state) {
    case 1:
        probe_calls++;
        if (!enough)
            goto skip_via_state2;
        result = 100;
        if (0)
        case 2:
        {
            result = 200;
        }
        result += 1;
        return result;
    skip_via_state2:
        return -2;
    }
    return -1;
}

static int test_minimal_duff(void) {
    /* Enter via case 1 with enough=1: `if(0)` must be false at RUNTIME
     * fall-through, so case 2's body must NOT execute -- result stays
     * 100, then +1 = 101, not 200+1=201. */
    int r1 = minimal(1, 1);
    assert(r1 == 101);

    /* Enter directly via case 2 (switch dispatch bypasses the if(0)
     * entirely): case 2's body DOES execute -- result = 200, then
     * +1 = 201. */
    int r2 = minimal(2, 0);
    assert(r2 == 201);

    return 0;
}

/* --- test 2: LZ4-shaped state machine -- two states, one Duff jump --- */
/* Mirrors lz4frame.c's dstage_getCBlock / dstage_storeCBlock pair: state
 * A checks a condition and, if satisfied, falls through into state B's
 * body directly (skipping state B's own entry gate) via `if (0) case B:`.
 * If state A's condition fails, state B must NOT run this call -- the
 * caller re-enters later with state set to B directly. */

enum { STAGE_A, STAGE_B };

static int stage_b_body_ran;

static int state_machine(int *stage, int haveEnough) {
    for (;;) {
        switch (*stage) {
        case STAGE_A:
            if (!haveEnough) {
                *stage = STAGE_B;   /* caller must resupply, re-enter as B */
                return 0;           /* nothing produced yet */
            }
            /* enough data: proceed directly into STAGE_B's body without
             * re-checking its own entry conditions (Duff jump) */
            if (0)
        case STAGE_B:
            {
                stage_b_body_ran++;
            }
            *stage = STAGE_A;
            return 1;               /* produced one unit */
        }
    }
}

static int test_lz4_shaped_state_machine(void) {
    int stage = STAGE_A;
    stage_b_body_ran = 0;

    /* Not enough data: STAGE_A defers to STAGE_B, returns immediately.
     * STAGE_B's body must NOT have run via fall-through. */
    int r = state_machine(&stage, 0);
    assert(r == 0);
    assert(stage == STAGE_B);
    assert(stage_b_body_ran == 0);

    /* Now resupplied: re-enter directly as STAGE_B via the switch
     * dispatch (bypassing STAGE_A's condition, as intended). */
    r = state_machine(&stage, 1);
    assert(r == 1);
    assert(stage == STAGE_A);
    assert(stage_b_body_ran == 1);

    return 0;
}

int main(void) {
    int f0 = test_minimal_duff();
    int f1 = test_lz4_shaped_state_machine();
    return f0 | f1;
}
