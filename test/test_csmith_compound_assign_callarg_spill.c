/* csmith 1105826-fail_O2_optmismatch_356.c (reduced): a compound
 * assignment `(*p) ^= f()` nested deep inside another call's argument
 * expression, under -O2/-O3 (opt.c's -finline).
 *
 * gen_funcall() estimates each argument's own internal register need
 * via expr_reg_pressure() to decide whether to stage already-computed
 * earlier arguments to memory (use_staging) before evaluating a later,
 * more complex one. expr_reg_pressure() had no cases for ND_ASSIGN,
 * ND_COMMA, ND_DEREF, or ND_ADDR, so they fell through to `default:
 * return 0` -- a compound assignment (parser.c's to_assign() desugars
 * `*p ^= f()` into `(tmp = &*p), (*tmp = f() ^ *tmp)`) was scored as
 * needing zero registers, even though the address in `tmp` must stay
 * live in its own register across the whole rhs evaluation.
 *
 * With the estimate too low, gen_funcall() under-provisioned headroom
 * and the ND_BITXOR lowering's r_lhs==r_rhs same-register-reuse path
 * fired without the register actually having been spilled first: the
 * inlined f() result and the `*p` load landed in the same register,
 * and `xor reg, reg` self-zeroed instead of computing `f() ^ *p`.
 *
 * Fixed in codegen.c's expr_reg_pressure(): ND_ASSIGN/ND_COMMA now
 * charge like any other binary op, and ND_DEREF/ND_ADDR each add 1 for
 * the address they keep live.
 */
#include <stdio.h>
#include <stdint.h>

static int16_t lshift_left;
static int lshift_right;
/* single-return body: eligible for -finline's inlining */
static int16_t inline_lshift(void) { return (int16_t)(lshift_left << lshift_right); }

/* never actually called (0 args always route past every guard below);
 * only its call-argument setup matters */
static int sub_dummy(int a, int16_t b) { (void)a; (void)b; return 0; }

static int32_t g = 0x8019725D;

static void run(void) {
    int32_t *p = &g;
    sub_dummy(
        0,
        ((((((
              (((
                  ((((((
                         ((((0x877CE653DD91D589LL &
                             ((((((0xBE7DL ^
                                   (((
                                        (((
                                            (((*p) ^=
                                              (inline_lshift())) >
                                             0) ))) )))) ) ) >
                                0x92081B8FL) ))) ) >= 0) )))) ||
                     0x6BL) )) ))))))) ) >= 0xBE7DL));
}

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

int main(void) {
    lshift_left = 0x29C6;
    lshift_right = 1;
    run();
    /* g ^= (int16_t)(0x29C6 << 1) == g ^ 0x538C; low byte -> 0x5D */
    CHECK(g == (int32_t)(0x8019725D ^ (int16_t)(0x29C6 << 1)));
    if (fail) { fprintf(stderr, "%d check(s) failed\n", fail); return 1; }
    printf("OK\n");
    return 0;
}
