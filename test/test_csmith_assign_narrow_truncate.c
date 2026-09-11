/* csmith 2579432 (reduced_refmismatch_O0_44.c) and csmith 2278747
 * (reduced_refmismatch_O0_{79,84}.c): the VALUE of an assignment
 * expression `(lhs = rhs)` must have lhs's type (C11 6.5.16p3), not
 * rhs's raw width. `*g = (b[1][4][4] = e) & safe_sub_func_uint16_t_u_u(f, 1)`
 * with `uint8_t b[...]` and `uint32_t e` needs `b[1][4][4] = e` to
 * contribute the 8-bit truncated store, not the full 32-bit `e`.
 *
 * Two codegen paths built the assignment's result register from the
 * raw rhs without narrowing/sign-extending it to the lhs type:
 *  - the local-scalar-variable fast path only masked UNSIGNED narrow
 *    (<4 byte) locals, never sign-extended signed narrow locals;
 *  - the generic fallback path (globals, array elements, pointer
 *    derefs) did no truncation/sign-extension at all.
 *
 * Fixed in codegen.c's ND_ASSIGN scalar-store cases: after the store,
 * mask (unsigned) or movsx (signed) narrow (<4 byte) integer results
 * back to the lhs type before returning them as the expression value.
 */
#include <stdio.h>
#include <stdint.h>

static uint16_t safe_sub_func_uint16_t_u_u(uint16_t a, uint16_t b) { return (uint16_t)(a - b); }

static int8_t garr[4];
static uint8_t garru[4];

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

uint8_t b[8][5][5];
int64_t c;

static void d(int16_t x, int16_t y) {
    (void)x; (void)y;
    uint32_t e = 4073709551607u; /* truncates to 0x7C02BFF7 mod 2^32 */
    int16_t f = 324;
    int64_t *g = &c;
    *g = (b[1][4][4] = e) & safe_sub_func_uint16_t_u_u(f, 1);
}

int main(void) {
    d(0, 0);
    /* b[1][4][4] must store e truncated to uint8_t (0xF7 -> 247), and the
     * assignment expression's VALUE used in the `&` must be that same
     * truncated 247, not the untruncated 32-bit e. */
    CHECK(b[1][4][4] == 247);
    CHECK(c == (247 & (int)(uint16_t)(324 - 1)));

    /* Global/array signed narrow assignment expression value must be
     * sign-extended, not left as the raw (unsigned-looking) rhs. */
    int r = (garr[0] = 200) + 1; /* (int8_t)200 == -56 */
    CHECK(r == -55);

    /* Local signed narrow assignment expression value, same rule. */
    int8_t loc;
    int r2 = (loc = 200) + 1;
    CHECK(r2 == -55);

    /* Global/array unsigned narrow assignment expression value must be
     * masked to the lhs width. */
    int r3 = (garru[0] = 4073709551607u) + 1; /* (uint8_t)0x...B7 == 247 */
    CHECK(r3 == 248);

    if (fail) { fprintf(stderr, "%d check(s) failed\n", fail); return 1; }
    printf("OK\n");
    return 0;
}
