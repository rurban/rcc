/* csmith 1513742: scalar store through a spilled rhs register.
 *
 * In the ND_ASSIGN scalar fallback, rcc computed the rhs into VReg r2, then
 * gen_addr(lhs) for a chained deref (***p) exhausted the register file,
 * spilled r2's physical register, and reused that same slot for the
 * address (VReg identity IS the physical register index). The final
 * `mov r2, (r1)` then degenerated into a self-store `mov %rsi,(%rsi)`,
 * writing the address into the target instead of the rhs value.
 *
 * Every other binary-op lowering in codegen.c already guards the
 * r_lhs == r_rhs && spilled case by reloading the spill slot; the plain
 * scalar assignment path was the only one missing the guard.
 *
 * Found via test/csmith/1513742-reduced_{ref,opt}mismatch_{O0_78,*_41}.c.
 */
#include <stdio.h>
#include <stdint.h>

static int32_t safe_sub_func_int32_t_s_s(int32_t si1, int32_t si2){return si1-si2;}
static int64_t safe_sub_func_int64_t_s_s(int64_t si1, int64_t si2){return si1-si2;}
static uint32_t safe_lshift_func_uint32_t_u_u(uint32_t left, unsigned int right){return left<<right;}
static int32_t safe_div_func_int32_t_s_s(int32_t si1, int32_t si2){return si2==0?si1:si1/si2;}
static int32_t safe_mul_func_int32_t_s_s(int32_t si1, int32_t si2){return si1*si2;}
static int64_t safe_rshift_func_int64_t_s_u(int64_t left, unsigned int right){return left>>right;}
static uint64_t safe_div_func_uint64_t_u_u(uint64_t ui1, uint64_t ui2){return ui2==0?ui1:ui1/ui2;}

static uint64_t g_40 = 5;
static uint64_t *g_106 = &g_40;
static uint64_t **g_105 = &g_106;
static uint64_t ***g_362 = &g_105;
static int64_t g_9val = 7;
static int64_t *g_595 = &g_9val;
static int64_t **g_1270 = &g_595;
static int64_t * volatile *g_594 = (int64_t* volatile*)&g_595;
static const uint16_t g_966val = 3;
static const uint16_t *g_966 = &g_966val;
static const uint16_t **g_965 = &g_966;
static uint16_t g_259 = 2;
static uint8_t g_645val = 1;
static uint8_t *g_645 = &g_645val;
static const int32_t g_281val = 4;
static const int32_t *g_281 = &g_281val;
static uint32_t g_165[2][2] = {{1,2},{3,4}};
static long g_1037 = 0;
static int16_t g_176 = 7;

static uint64_t g_338 = 0;
static uint64_t *g_1488 = &g_338;
static uint64_t ** const g_1487 = &g_1488;
static uint64_t ** const *g_1486 = &g_1487;

static int32_t g_274ptr = 42;
static int32_t *g_274v = &g_274ptr;
static int32_t **g_274 = &g_274v;

static int32_t g_791val = 11;
static int32_t *g_791 = &g_791val;

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

static int32_t * func_22(int32_t * p_23, int32_t * p_24) {
    uint16_t l_1509 = 3UL;
    int8_t l_1529val = 9;
    int8_t *l_1529 = &l_1529val;
    int32_t l_1514 = 9L;
    int32_t l_43 = 0x87D17319L;
    int8_t l_1530 = (-3L);
    l_1514 ^= ((((safe_mul_func_int32_t_s_s((((**g_594) = ((safe_rshift_func_int64_t_s_u(((void*)0 != &l_1509), 59)) ^ ((*l_1529) = (safe_div_func_uint64_t_u_u(((**g_965) != ((((((**g_1270) | ((**g_105) = (safe_sub_func_int32_t_s_s((0UL <= (***g_362)), (safe_sub_func_int64_t_s_s((safe_lshift_func_uint32_t_u_u(g_165[1][0], 24)), (safe_div_func_int32_t_s_s(((((((((((g_1037 , (((***g_1486) = ((*p_24) & g_176)) || 0x3E183EEF7194CFA9LL)) && 0xC9L) | (**g_1270)) & (*p_24)) > 1UL) >= (**g_274)) == g_259) != (*g_645)) < (***g_362)) > 1UL), (*g_281))))))))) == 0UL) && g_40) , 0x943AL) & (**g_965))), (**g_594)))))) || l_43), (*g_791))) ^ l_1530) | 1UL) , (*p_24));
    return p_24;
}

int main(void) {
    int32_t p23v = 1, p24v = 100;
    func_22(&p23v, &p24v);
    /* The store under test: ***g_1486 = (*p_24) & g_176 == 100 & 7 == 4.
     * The pre-fix compiler stored the address of g_338 instead. */
    CHECK(g_338 == (uint64_t)((int32_t)(p24v & g_176)));
    if (fail) { fprintf(stderr, "%d check(s) failed\n", fail); return 1; }
    printf("OK\n");
    return 0;
}
