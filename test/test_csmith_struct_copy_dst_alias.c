/* csmith 1105826-reduced_refmismatch_O0_145 (non-crashing form): rcc -O0
 * silently dropped a chained struct assignment `g_50 = ((*l_2145) = l_2144)`
 * nested deep inside a large boolean/arithmetic expression -- g_50 kept its
 * initializer instead of picking up the copied value.
 *
 * Root cause in codegen.c's ND_ASSIGN struct/union/array copy path: `dst`
 * (the lhs address) is computed first, then `src` (the rhs) is evaluated.
 * Under the surrounding expression's register pressure, evaluating `src`
 * needed a register and the allocator picked `dst`'s register as the spill
 * victim -- correctly saving dst's address to its spill slot, but then
 * handing that SAME physical register number to `src`. Since a VReg IS its
 * physical register index, `dst` and `src` silently became the same VReg;
 * the byte-copy loop that followed read and wrote through `src`'s address
 * for BOTH operands (`movb -1(%src,%cnt),%al` / `movb %al,-1(%src,%cnt)`),
 * a no-op self-copy that left the real destination (dst's address, still
 * sitting forgotten in its spill slot) untouched.
 *
 * Fixed by protect_from_addr_alias(dst, dst) right after src is computed:
 * a no-op unless dst's register is currently marked spilled, in which case
 * it reloads dst's original address into a fresh register instead of
 * silently reading whatever src left behind.
 *
 * Manual minimization lost the exact register-pressure trigger (like the
 * other test_csmith_*_spill.c cases), so this keeps the full expression
 * shape from the reduced csmith source, with values pinned to concrete
 * constants instead of state built up by surrounding csmith boilerplate.
 */
#include <stdint.h>
#include <stdio.h>

static int8_t safe_mul_func_int8_t_s_s(int8_t a, int8_t b) { return (int8_t)(a * b); }
static int8_t safe_rshift_func_int8_t_s_u(int8_t left, unsigned int right) {
    return (left < 0 || right >= 32) ? left : (int8_t)(left >> right);
}
static int64_t safe_mod_func_int64_t_s_s(int64_t a, int64_t b) {
    return (b == 0 || (a == (-9223372036854775807LL - 1) && b == -1)) ? a : a % b;
}
static uint8_t safe_sub_func_uint8_t_u_u(uint8_t a, uint8_t b) { return (uint8_t)(a - b); }
static uint8_t safe_lshift_func_uint8_t_u_s(uint8_t left, int right) {
    return ((right < 0) || (right >= 32) || (left > (255 >> right))) ? left : (uint8_t)(left << right);
}
static uint64_t safe_div_func_uint64_t_u_u(uint64_t a, uint64_t b) { return b == 0 ? a : a / b; }
static uint8_t safe_div_func_uint8_t_u_u(uint8_t a, uint8_t b) { return b == 0 ? a : (uint8_t)(a / b); }
static int16_t safe_mul_func_int16_t_s_s(int16_t a, int16_t b) { return (int16_t)(a * b); }

struct S0 {
    unsigned f0 : 30;
};

static struct S0 g_50;
static struct S0 l_2065 = {16021};
static struct S0 l_2144 = {5143};
static int32_t dummy_l2063 = 1;
static int32_t *p_l2063 = &dummy_l2063;
static int32_t **l_2063 = &p_l2063;
static int16_t g_238 = 14550;
static int16_t *l_2147 = &g_238;
static int8_t l_2091 = 44;
static uint8_t g_169 = 46;
static struct S0 g_2139 = {4294967294u & 0x3FFFFFFFu};
static uint8_t g_1475v = 247;
static const uint8_t *g_1475 = &g_1475v;
static uint8_t g_328 = 84;
static uint8_t *l_2140 = &g_328;
static uint16_t g_665 = 65530;
static uint16_t *g_1287 = &g_665;
static const int16_t g_2148 = -3;
static int32_t l_2149[4][10];
static int32_t l_2115[8] = {-1875420820, -1875420820, -1875420820, -1875420820,
                             -1875420820, -1875420820, -1875420820, -1875420820};
static int64_t l_2150 = 6;
static uint32_t g_8v = 3220458668u;
static uint32_t *g_8 = &g_8v;
static int64_t g_666 = 0;
static int64_t *g_703 = &g_666;
static uint16_t l_2151[5][8][3];
static const struct S0 l_2108[4];

struct S0 *l_2145 = &l_2065;

static int fail;
#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond);                 \
            fail++;                                                                 \
        }                                                                           \
    } while (0)

int main(void) {
    CHECK(g_50.f0 == 0);
    l_2144 = ((safe_mod_func_int64_t_s_s(
                  ((safe_mul_func_int8_t_s_s(
                       (((safe_lshift_func_uint8_t_u_s(
                              (safe_lshift_func_uint8_t_u_s(
                                  (g_169 = ((-9L) & ((safe_rshift_func_int8_t_s_u(
                                                          (safe_sub_func_uint8_t_u_u(
                                                              ((safe_div_func_uint64_t_u_u(
                                                                   ((safe_div_func_uint8_t_u_u((**l_2063), ((*l_2140) = (g_2139.f0, (*g_1475))))) ==
                                                                    (((safe_mul_func_int16_t_s_s(
                                                                           ((*l_2147) = ((*g_1475) & ((l_2091, (~((*g_8) = (*g_8)))) |
                                                                                                       ((g_50 = ((*l_2145) = l_2144)),
                                                                                                        ((((((*g_703) = (*g_703)) && 0UL) && l_2115[2]), 1UL) && g_169))))),
                                                                           (*g_1287))) >=
                                                                       0xC58AL) == g_2148)),
                                                                   l_2149[3][9])) < 0x9328A2DDD9115465LL),
                                                              l_2149[3][9])),
                                                          l_2091)) ^
                                                      l_2150))),
                                  l_2115[3])),
                              0)),
                        l_2115[5]) >= 0),
                       0)) ||
                   249UL),
                  l_2151[0][0][1])),
              l_2108[0]);
    /* g_50 must pick up l_2144's PRE-statement value (5143) via the chained
     * assignment `g_50 = ((*l_2145) = l_2144)`, unaffected by the OUTER
     * `l_2144 = ...` assignment that only completes after the whole rhs
     * (including this nested read of l_2144) has been evaluated. */
    CHECK(g_50.f0 == 5143);
    CHECK(l_2144.f0 == 0);
    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
