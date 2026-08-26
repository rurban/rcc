/* codegen.c compiles every function TWICE: Pass 1 (cg_dry_run=true) walks
 * the body purely to discover register/stack usage; Pass 2 (cg_dry_run=
 * false) walks it again to emit the real code, reusing Pass 1's
 * measurements to size the stack frame ahead of time.
 *
 * gen_funcall()'s ordinary (non-inline-asm) call-emission path protects
 * any value still live in the two caller-saved scratch virtual registers
 * (vreg 0/%r10, vreg 1/%r11) across a call by spilling it to a stack
 * slot via spill_offset(0)/spill_offset(1) before the `call` and
 * reloading via the SAME spill_offset() call after it. spill_offset()
 * only PUSHES a fresh slot the first time a given register needs
 * protection in the current pass; every later call site in that pass
 * reuses the cached slot (correct: those protections never overlap in
 * time). This means spill_depth[0]/spill_depth[1] go from 0 to 1 the
 * first time either register needs protecting and then stay at 1 for
 * the rest of that pass -- they are never popped back to 0.
 *
 * Bug: spill_slot[][]/spill_depth[] (memset to 0 once, right before
 * Pass 1 begins) were never reset before Pass 2 starts. Pass 2 inherits
 * Pass 1's *final* depth/slot state. Its own first spill_offset(r) call
 * then sees a stale spill_depth[r] > 0 and returns spill_slot[r][...]
 * -- whatever small, un-reanchored offset Pass 1 happened to record --
 * instead of pushing a fresh slot anchored above the just-computed
 * frame size (see the next_spill_slot re-anchor a few lines below the
 * fix). When that stale offset happens to alias a live parameter's own
 * stack slot, the very next call needing scratch-register protection
 * silently overwrites the parameter with garbage.
 *
 * Found via mbedtls/tf-psa-crypto's psa_crypto-suite: test_mac_sign()'s
 * `data_t *input` parameter got its slot overwritten mid-function
 * (confirmed via disassembly: `mov %r11, -0x80(%rbp)` immediately
 * before `call psa_import_key`, where -0x80(%rbp) was ALSO `input`'s
 * own parameter-spill slot from the prologue) -- SIGSEGV on the next
 * `input->x` dereference. Fixed by resetting spill_slot[]/spill_depth[]
 * at the start of Pass 2, alongside the other per-pass state
 * (used_regs/spilled_regs/reg_owner) already reset there.
 *
 * This case reliably drives spill_depth[0] and spill_depth[1] to 1 by
 * Pass 1's end (confirmed via internal instrumentation during
 * development: both nested nonvoid nonvoid calls below need every
 * caller-saved scratch register protected across an inner call). Its
 * parameters, like every function's, start well above the offsets
 * spill_offset() hands out cold -- reliably reproducing this exact
 * numeric collision in a small, environment-independent test requires
 * matching register-allocation decisions no simpler repro reproduced
 * (many were tried); the real-world mbedtls SIGSEGV above is the
 * authoritative reproduction for the fix itself. This test still
 * exercises -- and must keep passing through -- the exact vulnerable
 * pattern (multiple nested-call sites each needing r10/r11 live-value
 * protection within one function), guarding against the state leak
 * recurring even where it can't itself observe a corrupted value.
 */

typedef struct { unsigned char *x; long len; } data_t;

static int f0(void) { return 1; }
static int f1(void) { return 2; }
static int f2(void) { return 3; }
static int f3(void) { return 4; }
static int f4(void) { return 5; }
static int f5(void) { return 6; }
static int f6(void) { return 7; }
static int f7(void) { return 8; }
static int f8(void) { return 9; }
static int f9(void) { return 10; }
static int f10(void) { return 11; }
static int f11(void) { return 12; }

static int combine(int a, int b) { return a * 31 + b; }

static int lookup_key(void *attrs, unsigned char *d, long l, int *k) {
    (void)attrs; (void)d; (void)l;
    *k = 7;
    return 0;
}

static int test_fn(int key_type, data_t *key_data, int alg,
                    data_t *input, data_t *expected) {
    int key = 0;
    (void)key_type;
    (void)alg;
    long e = expected->len;

    /* Each combine() call site needs its own two operands, each of
     * which is itself a call result added to a live memory value --
     * this drives r10/r11 protection at multiple call sites within
     * the same function. */
    int g1 = combine((int)(e + f0()) + (int)(e + f1()), (int)(e + f2()) + (int)(e + f3()));
    int g2 = combine((int)(e + f4()) + (int)(e + f5()), (int)(e + f6()) + (int)(e + f7()));
    int g3 = combine(g1, g2);
    int g4 = combine((int)(e + f8()) + (int)(e + f9()), (int)(e + f10()) + (int)(e + f11()));
    int total = combine(g3, g4);
    if (total == 0x7fffffff) return 1; /* never true; keeps total live */

    if (lookup_key(0, key_data->x, key_data->len, &key) != 0) return 1;
    if (key != 7) return 1;

    /* The regression: `input`'s own parameter slot must survive every
     * scratch-register-protecting call above untouched. */
    if (!input->x) return 1;
    return input->x[0] == 5 ? 0 : 1;
}

int main(void) {
    unsigned char kd[4] = {1, 2, 3, 4};
    unsigned char id[4] = {5, 6, 7, 8};
    unsigned char ed[28] = {0};
    data_t key_data = {kd, 4};
    data_t input = {id, 4};
    data_t expected = {ed, 28};
    return test_fn(6, &key_data, 1, &input, &expected);
}
