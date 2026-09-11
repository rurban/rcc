/* csmith 1105826 (reduced_refmismatch_O0_145.c, reduced further to a crash-only
 * repro): rcc -O0 segfaults compiling+running a chained struct assignment
 * `g_50 = ((*l_2145) = l_2144)` (both struct S0, a 30-bit unsigned bitfield)
 * nested deep inside a large boolean/arithmetic expression.
 *
 * Root cause in codegen.c's ND_ASSIGN struct/union/array copy path: the
 * byte-copy loop's counter register was picked with a plain `alloc_reg()`
 * ("dedicated counter, doesn't conflict with src/dst" -- but nothing
 * enforced that). Under the register pressure of this expression, alloc_reg()
 * sometimes handed the counter the SAME physical register as `src` (or
 * `dst`), so the loop's `movb -1(%src,%cnt,1),%al` addressed `2*cnt-1`
 * instead of a real pointer -- a near-zero address, segfaulting once
 * `cnt` counted down from the struct's size. Fixed by allocating the
 * counter with `alloc_reg_avoid2(src, dst)` instead.
 *
 * Manual minimization lost the exact register-pressure trigger (like the
 * other test_csmith_*_spill.c cases), so this keeps the full reduced
 * csmith source. Not crashing is the only assertion: run to completion.
 */
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
static void platform_main_end(uint32_t crc, int flag);
static int8_t(safe_mul_func_int8_t_s_s)(int8_t si1, int8_t si2) {}
static int8_t(safe_mod_func_int8_t_s_s);
static int8_t(safe_rshift_func_int8_t_s_u)(int8_t left, unsigned int right) {}
static int16_t(safe_mul_func_int16_t_s_s)(int16_t si1, int16_t si2) {}
static int16_t(safe_lshift_func_int16_t_s_u);
static int64_t(safe_add_func_int64_t_s_s);
static int64_t(safe_mod_func_int64_t_s_s)(int64_t si1, int64_t si2) {}
static uint8_t(safe_sub_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2) {}
static uint8_t(safe_div_func_uint8_t_u_u);
static uint8_t(safe_lshift_func_uint8_t_u_s)(uint8_t left, int right) {}

static uint16_t(safe_rshift_func_uint16_t_u_u);
static uint32_t(safe_mod_func_uint32_t_u_u);
static uint64_t(safe_div_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2) {}
static uint32_t crc32_tab;
static uint32_t crc32_context = 0xFFFFFFFFUL;
static void crc32_gentab(void);
static void crc32_byte(uint8_t b);
static void crc32_8bytes(uint64_t val);
static void transparent_crc(uint64_t val, char *vname, int flag);
struct S0 {
    unsigned f0 : 30;
};
struct S1 {
    const int32_t f0;
    int32_t f2;
    uint32_t f3;
    const uint32_t f5;
};
static uint32_t g_9 = 0xBFF44CACL;

static int8_t g_38;
static struct S0 g_50;

static int32_t g_90 = 1L;

static struct S1 g_130;
static int32_t *g_133 = &g_90;
static uint8_t g_169 = 0x2EL;
static int32_t **g_228 = &g_133;

static int16_t g_238 = 0x38D6L;
static uint8_t g_328 = 0x54L;
static struct S1 g_410;

static int8_t g_502;
static struct S1 g_587;
static uint16_t g_665 = 65530UL;
static int64_t g_666;

static int8_t *g_772;

static int32_t *g_1137 = &g_90;
static const int32_t *g_1171 = &g_90;
static uint16_t *g_1287 = &g_665;

static const uint8_t g_1476 = 0xF7L;

static struct S1 g_1596;
static struct S1 g_1597;

static const int16_t g_2148 = 0;

static uint16_t func_1(void) {
    int32_t **l_2063 = &g_1137;
    struct S0 l_2065;

    const struct S0 l_2108[4];
    uint16_t l_2151[5][8][3];
    const struct S0 **l_2166[9];

    if ((0, (**l_2063))) {
        int8_t l_2091 = 0x2CL;

        for (g_410.f2 = 2;; g_410.f2 -= 1) {
            int8_t l_2074 = 0;
            int32_t l_2115[8];
            struct S0 l_2144;
            int32_t l_2149[4][10];
            int64_t l_2150 = 6L;

            for (g_1596.f3 = 0;; g_1596.f3 += 1) {

                struct S0 *l_2145 = &l_2065;
                int16_t *l_2147 = &g_238;

                for (g_1597.f3 = 0; 0; g_130.f3 += 1)
                    ;
                l_2144 = ((safe_mod_func_int64_t_s_s(
                              ((safe_mul_func_int8_t_s_s(
                                   (((safe_lshift_func_uint8_t_u_s(
                                          (safe_lshift_func_uint8_t_u_s(
                                              (g_169 = (0 & ((safe_rshift_func_int8_t_s_u(
                                                                  (safe_sub_func_uint8_t_u_u(
                                                                      ((safe_div_func_uint64_t_u_u(
                                                                           (0 == (((safe_mul_func_int16_t_s_s(
                                                                                       ((*l_2147) = (0 & (0 | ((g_50 = ((*l_2145) = l_2144)), 0)))), 0)) >=
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
                for (l_2074 = 0;; l_2074 += 1)
                    return 0;
            }
        }
    }
    for (g_587.f3 = 0; 0; g_587.f3 = 0)
        ;
}
int main(int argc, char *argv[]) {
    ;
    func_1();
    ;
    ;
}
