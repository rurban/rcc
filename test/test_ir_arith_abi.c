// Differential regression test for the -O2 IR emitters (instruction selection, immediates,
// extensions, compare/branch conditions, FP, spills across calls, ABI argument/return
// placement, large frames and displacements). Every section folds its results into a
// checksum that is compared with the value a reference compiler produced (clang -O0
// -ffp-contract=off, run on the same operand tables), so a wrong immediate range, a wrong
// condition code, a missing extension or a clobbered register shows up as a mismatching section.
//
// Regenerate the table (with a reference compiler): compile with -DIR_ARITH_GEN, run, paste the
// printed lines into `expected`. Compile with -DIR_AGG_EXOTIC as well to also run the aggregate
// placements the AST backend of some targets gets wrong (the last table entry).
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NOINL __attribute__((noinline))

static uint64_t h = 1469598103934665603ULL;
static int fails, section_no;
static void mix(uint64_t v) { h = (h ^ v) * 1099511628211ULL; }
static void mixf(double d) {
    uint64_t b;
    if (d != d) b = 0x7ff8000000000000ULL; // all NaNs alike
    else
        memcpy(&b, &d, 8);
    mix(b);
}

#ifdef IR_ARITH_GEN
#define CHECK(name) \
    do { \
        printf("    0x%016llxULL, // %s\n", (unsigned long long)h, name); \
        h = 1469598103934665603ULL; \
    } while (0)
#else
static const uint64_t expected[] = {
    0xa2cf71e566422727ULL, // reg32
    0x9345dc3be983d406ULL, // reg64
    0xc1b9be02f6af30f7ULL, // imm32
    0xf0c3d2ddb9d6d1e1ULL, // imm64
    0x7cabe7cb355988b8ULL, // cmp32
    0x1ed8cfda60066642ULL, // cmp64
    0x753df8ed13c34580ULL, // cmpf
    0xf63a385fe53ca023ULL, // ext
    0x173772451e93a0f9ULL, // fp
    0x14c94e43832739dfULL, // calls
    0xaaed1b2706a37706ULL, // frames
    0xd7411ccca114301dULL, // aggregates
    0xcc0d5c2973322dfdULL, // aggregates_exotic
};
#define CHECK(name) \
    do { \
        if (h != expected[section_no]) { \
            printf("FAIL section %s: got %016llx expected %016llx\n", name, (unsigned long long)h, \
                   (unsigned long long)expected[section_no]); \
            fails++; \
        } \
        section_no++; \
        h = 1469598103934665603ULL; \
    } while (0)
#endif

static const int32_t v32[] = {0, 1, -1, 2, 3, 7, 31, 32, 63, 64, 100, -100, 255, 256, 4095, 4096, 4097, 65535, 65536,
                              0x7fffffff, (int32_t)0x80000000, (int32_t)0x80000001, 0x12345678, (int32_t)0xdeadbeef,
                              0x00ff00ff, (int32_t)0xff00ff00, 0x55555555, (int32_t)0xaaaaaaaa, -4096, -4097, -65536};
static const int64_t v64[] = {0, 1, -1, 2, 3, 63, 64, 4095, 4096, 4097, -4096, -4097, 0xffffffffLL, 0x100000000LL,
                              0x7fffffffffffffffLL, (int64_t)0x8000000000000000ULL, (int64_t)0x8000000000000001ULL,
                              0x123456789abcdef0LL, (int64_t)0xdeadbeefcafebabeULL, 0x00ff00ff00ff00ffLL,
                              (int64_t)0xff00ff00ff00ff00ULL, 0x5555555555555555LL, (int64_t)0xaaaaaaaaaaaaaaaaULL,
                              -65536, 0xfff000LL, 0x1000000LL, -0x1000000LL, 123456789012LL};
#define N32 (int)(sizeof v32 / sizeof v32[0])
#define N64 (int)(sizeof v64 / sizeof v64[0])

// ---------------------------------------------------------------- register-operand integer ops
#define BIN(name, T, expr) NOINL static T name(T a, T b) { return expr; }
BIN(add32, int32_t, a + b) BIN(sub32, int32_t, a - b) BIN(mul32, int32_t, (int32_t)((uint32_t)a * (uint32_t)b))
BIN(and32, int32_t, a & b) BIN(or32, int32_t, a | b) BIN(xor32, int32_t, a ^ b)
BIN(shl32, uint32_t, a << (b & 31)) BIN(shr32, uint32_t, a >> (b & 31)) BIN(sar32, int32_t, a >> (b & 31))
BIN(add64, int64_t, (int64_t)((uint64_t)a + (uint64_t)b)) BIN(sub64, int64_t, (int64_t)((uint64_t)a - (uint64_t)b))
BIN(mul64, int64_t, (int64_t)((uint64_t)a * (uint64_t)b)) BIN(and64, int64_t, a & b) BIN(or64, int64_t, a | b)
BIN(xor64, int64_t, a ^ b) BIN(shl64, uint64_t, a << (b & 63)) BIN(shr64, uint64_t, a >> (b & 63))
BIN(sar64, int64_t, a >> (b & 63))
NOINL static int32_t sdiv32(int32_t a, int32_t b) { return (b == 0 || (a == INT32_MIN && b == -1)) ? 0 : a / b; }
NOINL static int32_t srem32(int32_t a, int32_t b) { return (b == 0 || (a == INT32_MIN && b == -1)) ? 0 : a % b; }
NOINL static uint32_t udiv32(uint32_t a, uint32_t b) { return b ? a / b : 0; }
NOINL static uint32_t urem32(uint32_t a, uint32_t b) { return b ? a % b : 0; }
NOINL static int64_t sdiv64(int64_t a, int64_t b) { return (b == 0 || (a == INT64_MIN && b == -1)) ? 0 : a / b; }
NOINL static int64_t srem64(int64_t a, int64_t b) { return (b == 0 || (a == INT64_MIN && b == -1)) ? 0 : a % b; }
NOINL static uint64_t udiv64(uint64_t a, uint64_t b) { return b ? a / b : 0; }
NOINL static uint64_t urem64(uint64_t a, uint64_t b) { return b ? a % b : 0; }
NOINL static int32_t neg32(int32_t a) { return (int32_t)(0u - (uint32_t)a); }
NOINL static int32_t not32(int32_t a) { return ~a; }
NOINL static int64_t neg64(int64_t a) { return (int64_t)(0ull - (uint64_t)a); }
NOINL static int64_t not64(int64_t a) { return ~a; }

static void test_reg_ops(void) {
    for (int i = 0; i < N32; i++) {
        mix((uint32_t)neg32(v32[i]));
        mix((uint32_t)not32(v32[i]));
        for (int j = 0; j < N32; j++) {
            int32_t a = v32[i], b = v32[j];
            mix((uint32_t)add32(a, b)), mix((uint32_t)sub32(a, b)), mix((uint32_t)mul32(a, b));
            mix((uint32_t)and32(a, b)), mix((uint32_t)or32(a, b)), mix((uint32_t)xor32(a, b));
            mix(shl32(a, b)), mix(shr32(a, b)), mix((uint32_t)sar32(a, b));
            mix((uint32_t)sdiv32(a, b)), mix((uint32_t)srem32(a, b));
            mix(udiv32((uint32_t)a, (uint32_t)b)), mix(urem32((uint32_t)a, (uint32_t)b));
        }
    }
    CHECK("reg32");
    for (int i = 0; i < N64; i++) {
        mix((uint64_t)neg64(v64[i]));
        mix((uint64_t)not64(v64[i]));
        for (int j = 0; j < N64; j++) {
            int64_t a = v64[i], b = v64[j];
            mix((uint64_t)add64(a, b)), mix((uint64_t)sub64(a, b)), mix((uint64_t)mul64(a, b));
            mix((uint64_t)and64(a, b)), mix((uint64_t)or64(a, b)), mix((uint64_t)xor64(a, b));
            mix(shl64(a, b)), mix(shr64(a, b)), mix((uint64_t)sar64(a, b));
            mix((uint64_t)sdiv64(a, b)), mix((uint64_t)srem64(a, b));
            mix(udiv64((uint64_t)a, (uint64_t)b)), mix(urem64((uint64_t)a, (uint64_t)b));
        }
    }
    CHECK("reg64");
}

// ---------------------------------------------------------------- immediate operands
#define IMM32(tag, C) \
    NOINL static uint32_t i32_##tag(uint32_t x) { \
        return (x + (uint32_t)(C)) ^ (x - (uint32_t)(C)) ^ (x & (uint32_t)(C)) ^ (x | (uint32_t)(C)) ^ \
            ((x ^ (uint32_t)(C)) * 3u) ^ ((uint32_t)(int32_t)x * (uint32_t)(C)); \
    }
#define IMM64(tag, C) \
    NOINL static uint64_t i64_##tag(uint64_t x) { \
        return (x + (uint64_t)(C)) ^ (x - (uint64_t)(C)) ^ (x & (uint64_t)(C)) ^ (x | (uint64_t)(C)) ^ \
            ((x ^ (uint64_t)(C)) * 3ull) ^ (x * (uint64_t)(C)); \
    }
IMM32(0, 0) IMM32(1, 1) IMM32(2, 2) IMM32(3, 3) IMM32(4, 4) IMM32(8, 8) IMM32(4095, 4095) IMM32(4096, 4096)
IMM32(4097, 4097) IMM32(fff000, 0xfff000) IMM32(1000000, 0x1000000) IMM32(1001000, 0x1001000)
IMM32(m1, -1) IMM32(m4095, -4095) IMM32(m4096, -4096) IMM32(m4097, -4097) IMM32(imax, 0x7fffffff)
IMM32(imin, 0x80000000u) IMM32(ff, 0xff) IMM32(ff00, 0xff00) IMM32(a5, 0xaaaaaaaau) IMM32(55, 0x55555555)
IMM32(0f, 0x0f0f0f0fu) IMM32(fe, 0xfffffffeu) IMM32(wrap, 0xc0000003u) IMM32(odd, 0x12345678)
IMM32(ff00ff, 0x00ff00ffu) IMM32(mask7, 0x7ffffffeu)
IMM64(0, 0) IMM64(1, 1) IMM64(4095, 4095) IMM64(4096, 4096) IMM64(4097, 4097) IMM64(fff000, 0xfff000)
IMM64(m1, -1) IMM64(m4096, -4096) IMM64(m4097, -4097) IMM64(lo32, 0xffffffffull) IMM64(hi32, 0xffffffff00000000ull)
IMM64(5, 0x5555555555555555ull) IMM64(imax, 0x7fffffffffffffffull) IMM64(imin, 0x8000000000000000ull)
IMM64(fe, 0xfffffffffffffffeull) IMM64(ff00ff, 0x00ff00ff00ff00ffull) IMM64(odd, 0x123456789abcdef0ull)
IMM64(movn, 0xffffffffffff1234ull) IMM64(mid, 0x0000ffff00000000ull) IMM64(wrap, 0xc000000000000003ull)
IMM64(big, 0x100000001ull) IMM64(shifted, 0x1000000ull)

static void test_imm_ops(void) {
    for (int i = 0; i < N32; i++) {
        uint32_t x = (uint32_t)v32[i];
        mix(i32_0(x)), mix(i32_1(x)), mix(i32_2(x)), mix(i32_3(x)), mix(i32_4(x)), mix(i32_8(x));
        mix(i32_4095(x)), mix(i32_4096(x)), mix(i32_4097(x)), mix(i32_fff000(x)), mix(i32_1000000(x));
        mix(i32_1001000(x)), mix(i32_m1(x)), mix(i32_m4095(x)), mix(i32_m4096(x)), mix(i32_m4097(x));
        mix(i32_imax(x)), mix(i32_imin(x)), mix(i32_ff(x)), mix(i32_ff00(x)), mix(i32_a5(x)), mix(i32_55(x));
        mix(i32_0f(x)), mix(i32_fe(x)), mix(i32_wrap(x)), mix(i32_odd(x)), mix(i32_ff00ff(x)), mix(i32_mask7(x));
    }
    CHECK("imm32");
    for (int i = 0; i < N64; i++) {
        uint64_t x = (uint64_t)v64[i];
        mix(i64_0(x)), mix(i64_1(x)), mix(i64_4095(x)), mix(i64_4096(x)), mix(i64_4097(x)), mix(i64_fff000(x));
        mix(i64_m1(x)), mix(i64_m4096(x)), mix(i64_m4097(x)), mix(i64_lo32(x)), mix(i64_hi32(x)), mix(i64_5(x));
        mix(i64_imax(x)), mix(i64_imin(x)), mix(i64_fe(x)), mix(i64_ff00ff(x)), mix(i64_odd(x)), mix(i64_movn(x));
        mix(i64_mid(x)), mix(i64_wrap(x)), mix(i64_big(x)), mix(i64_shifted(x));
    }
    CHECK("imm64");
}

// Constants returned as values must equal the same constants read from data.
#define RET64(tag, C) NOINL static uint64_t r64_##tag(void) { return C; }
#define RET32(tag, C) NOINL static uint32_t r32_##tag(void) { return C; }
RET64(a, 0x123456789abcdef0ull) RET64(b, 0xffffffffffff1234ull) RET64(c, 0xffff0000ffff0000ull)
RET64(d, 0x0000ffff00000000ull) RET64(e, 0xfffffffffffffffeull) RET64(f, 0x00000000ffff8000ull)
RET64(g, 0xffff8000ffffffffull) RET64(h, 0x0001000100010001ull) RET64(i, 0x8000000000000000ull)
RET64(j, 0xffffffff80000000ull) RET64(k, 0x00007fffffff0000ull) RET64(l, 0xabcd00000000ffffull)
RET32(a, 0x12345678u) RET32(b, 0xffff1234u) RET32(c, 0x1234ffffu) RET32(d, 0xfffffffeu) RET32(e, 0x80000000u)
RET32(f, 0x00ff00ffu) RET32(g, 0xffff0000u) RET32(h, 0x0000ffffu) RET32(i, 0x7fffffffu) RET32(j, 0xcafef00du)
static const uint64_t k64[] = {0x123456789abcdef0ull, 0xffffffffffff1234ull, 0xffff0000ffff0000ull, 0x0000ffff00000000ull,
                               0xfffffffffffffffeull, 0x00000000ffff8000ull, 0xffff8000ffffffffull, 0x0001000100010001ull,
                               0x8000000000000000ull, 0xffffffff80000000ull, 0x00007fffffff0000ull, 0xabcd00000000ffffull};
static const uint32_t k32[] = {0x12345678u, 0xffff1234u, 0x1234ffffu, 0xfffffffeu, 0x80000000u,
                               0x00ff00ffu, 0xffff0000u, 0x0000ffffu, 0x7fffffffu, 0xcafef00du};
static void test_const_return(void) {
    uint64_t g64[] = {r64_a(), r64_b(), r64_c(), r64_d(), r64_e(), r64_f(), r64_g(), r64_h(), r64_i(), r64_j(), r64_k(), r64_l()};
    uint32_t g32[] = {r32_a(), r32_b(), r32_c(), r32_d(), r32_e(), r32_f(), r32_g(), r32_h(), r32_i(), r32_j()};
    for (int i = 0; i < 12; i++)
        if (g64[i] != k64[i]) printf("FAIL const64 #%d: %016llx != %016llx\n", i, (unsigned long long)g64[i], (unsigned long long)k64[i]), fails++;
    for (int i = 0; i < 10; i++)
        if (g32[i] != k32[i]) printf("FAIL const32 #%d: %08x != %08x\n", i, g32[i], k32[i]), fails++;
}

// ---------------------------------------------------------------- compares and branches
#define CMPS(T, tag) \
    NOINL static int c_##tag##_lt(T a, T b) { return a < b; } \
    NOINL static int c_##tag##_le(T a, T b) { return a <= b; } \
    NOINL static int c_##tag##_gt(T a, T b) { return a > b; } \
    NOINL static int c_##tag##_ge(T a, T b) { return a >= b; } \
    NOINL static int c_##tag##_eq(T a, T b) { return a == b; } \
    NOINL static int c_##tag##_ne(T a, T b) { return a != b; } \
    NOINL static int b_##tag(T a, T b) { \
        int r = 0; \
        if (a < b) r |= 1; \
        if (a <= b) r |= 2; \
        if (a > b) r |= 4; \
        if (a >= b) r |= 8; \
        if (a == b) r |= 16; \
        if (a != b) r |= 32; \
        if (a < b && a != 0) r |= 64; \
        if (a > b || a == 5) r |= 128; \
        return r; \
    }
CMPS(int32_t, s32) CMPS(uint32_t, u32) CMPS(int64_t, s64) CMPS(uint64_t, u64) CMPS(double, f64) CMPS(float, f32)
#define CMPK(T, tag, id, K) \
    NOINL static int k_##tag##_##id(T a) { return (a < (T)(K)) | ((a <= (T)(K)) << 1) | ((a > (T)(K)) << 2) | ((a >= (T)(K)) << 3) | ((a == (T)(K)) << 4) | ((a != (T)(K)) << 5); } \
    NOINL static int kb_##tag##_##id(T a) { \
        int r = 0; \
        if (a < (T)(K)) r |= 1; \
        if (a <= (T)(K)) r |= 2; \
        if (a > (T)(K)) r |= 4; \
        if (a >= (T)(K)) r |= 8; \
        if (a == (T)(K)) r |= 16; \
        if (a != (T)(K)) r |= 32; \
        return r; \
    }
CMPK(int32_t, s32, 0, 0) CMPK(int32_t, s32, 1, 1) CMPK(int32_t, s32, 4095, 4095) CMPK(int32_t, s32, 4096, 4096) CMPK(int32_t, s32, 4097, 4097)
CMPK(int32_t, s32, fff000, 0xfff000) CMPK(int32_t, s32, m1, -1) CMPK(int32_t, s32, m4095, -4095) CMPK(int32_t, s32, m4096, -4096) CMPK(int32_t, s32, m4097, -4097)
CMPK(int32_t, s32, imax, 0x7fffffff) CMPK(int32_t, s32, odd, 0x12345)
CMPK(uint32_t, u32, 0, 0) CMPK(uint32_t, u32, 4095, 4095) CMPK(uint32_t, u32, 4097, 4097) CMPK(uint32_t, u32, fe, 0xfffffffeu) CMPK(uint32_t, u32, imin, 0x80000000u)
CMPK(int64_t, s64, 0, 0) CMPK(int64_t, s64, 1, 1) CMPK(int64_t, s64, 4095, 4095) CMPK(int64_t, s64, 4097, 4097) CMPK(int64_t, s64, m1, -1)
CMPK(int64_t, s64, m4097, -4097) CMPK(int64_t, s64, big, 0x100000000LL) CMPK(int64_t, s64, imax, 0x7fffffffffffffffLL)
CMPK(uint64_t, u64, 0, 0) CMPK(uint64_t, u64, 4096, 4096) CMPK(uint64_t, u64, max, 0xffffffffffffffffull) CMPK(uint64_t, u64, imin, 0x8000000000000000ull)
CMPK(double, f64, 0, 0) CMPK(double, f64, 1, 1) CMPK(double, f64, half, 0.5) CMPK(float, f32, 0, 0) CMPK(float, f32, 1, 1)

static const double vf[] = {0.0, -0.0, 1.0, -1.0, 0.5, 2.5, -2.5, 1e300, -1e300, 1e-300, 3.4e38, 1.0 / 3.0,
                            4294967295.0, 4294967296.0, 9223372036854775807.0, 18446744073709549568.0, 255.99, -255.99,
                            2147483647.0, -2147483648.0, 123456789.123, 0.1, 100.0, 31.0, 0.125, 16.0, -0.5};
#define NF (int)(sizeof vf / sizeof vf[0])
static double fnan(void) { volatile double z = 0.0; return z / z; }
static double finf(void) { volatile double z = 0.0; return 1.0 / z; }

static void test_compares(void) {
    for (int i = 0; i < N32; i++) {
        int32_t a = v32[i];
        mix(k_s32_0(a)), mix(k_s32_1(a)), mix(k_s32_4095(a)), mix(k_s32_4096(a)), mix(k_s32_4097(a)), mix(k_s32_fff000(a));
        mix(k_s32_m1(a)), mix(k_s32_m4095(a)), mix(k_s32_m4096(a)), mix(k_s32_m4097(a)), mix(k_s32_imax(a)), mix(k_s32_odd(a));
        mix(kb_s32_0(a)), mix(kb_s32_1(a)), mix(kb_s32_4095(a)), mix(kb_s32_4096(a)), mix(kb_s32_4097(a)), mix(kb_s32_fff000(a));
        mix(kb_s32_m1(a)), mix(kb_s32_m4095(a)), mix(kb_s32_m4096(a)), mix(kb_s32_m4097(a)), mix(kb_s32_imax(a));
        uint32_t u = (uint32_t)a;
        mix(k_u32_0(u)), mix(k_u32_4095(u)), mix(k_u32_4097(u)), mix(k_u32_fe(u)), mix(k_u32_imin(u));
        mix(kb_u32_0(u)), mix(kb_u32_4095(u)), mix(kb_u32_4097(u)), mix(kb_u32_fe(u)), mix(kb_u32_imin(u));
        for (int j = 0; j < N32; j++) {
            int32_t b = v32[j];
            mix(c_s32_lt(a, b)), mix(c_s32_le(a, b)), mix(c_s32_gt(a, b)), mix(c_s32_ge(a, b)), mix(c_s32_eq(a, b)), mix(c_s32_ne(a, b));
            mix(b_s32(a, b));
            mix(c_u32_lt((uint32_t)a, (uint32_t)b)), mix(c_u32_le((uint32_t)a, (uint32_t)b)), mix(c_u32_gt((uint32_t)a, (uint32_t)b));
            mix(c_u32_ge((uint32_t)a, (uint32_t)b)), mix(c_u32_eq((uint32_t)a, (uint32_t)b)), mix(c_u32_ne((uint32_t)a, (uint32_t)b));
            mix(b_u32((uint32_t)a, (uint32_t)b));
        }
    }
    CHECK("cmp32");
    for (int i = 0; i < N64; i++) {
        int64_t a = v64[i];
        mix(k_s64_0(a)), mix(k_s64_1(a)), mix(k_s64_4095(a)), mix(k_s64_4097(a)), mix(k_s64_m1(a)), mix(k_s64_m4097(a));
        mix(k_s64_big(a)), mix(k_s64_imax(a));
        mix(kb_s64_0(a)), mix(kb_s64_1(a)), mix(kb_s64_4095(a)), mix(kb_s64_4097(a)), mix(kb_s64_m1(a)), mix(kb_s64_m4097(a));
        uint64_t u = (uint64_t)a;
        mix(k_u64_0(u)), mix(k_u64_4096(u)), mix(k_u64_max(u)), mix(k_u64_imin(u));
        mix(kb_u64_0(u)), mix(kb_u64_4096(u)), mix(kb_u64_max(u)), mix(kb_u64_imin(u));
        for (int j = 0; j < N64; j++) {
            int64_t b = v64[j];
            mix(c_s64_lt(a, b)), mix(c_s64_le(a, b)), mix(c_s64_gt(a, b)), mix(c_s64_ge(a, b)), mix(c_s64_eq(a, b)), mix(c_s64_ne(a, b));
            mix(b_s64(a, b));
            mix(c_u64_lt((uint64_t)a, (uint64_t)b)), mix(c_u64_le((uint64_t)a, (uint64_t)b)), mix(c_u64_gt((uint64_t)a, (uint64_t)b));
            mix(c_u64_ge((uint64_t)a, (uint64_t)b)), mix(c_u64_eq((uint64_t)a, (uint64_t)b)), mix(c_u64_ne((uint64_t)a, (uint64_t)b));
            mix(b_u64((uint64_t)a, (uint64_t)b));
        }
    }
    CHECK("cmp64");
    double nan = fnan(), inf = finf();
    double pool[NF + 3];
    memcpy(pool, vf, sizeof vf);
    pool[NF] = nan, pool[NF + 1] = inf, pool[NF + 2] = -inf;
    for (int i = 0; i < NF + 3; i++) {
        double a = pool[i];
        mix(k_f64_0(a)), mix(k_f64_1(a)), mix(k_f64_half(a)), mix(kb_f64_0(a)), mix(kb_f64_1(a)), mix(kb_f64_half(a));
        float fa = (float)a;
        mix(k_f32_0(fa)), mix(k_f32_1(fa)), mix(kb_f32_0(fa)), mix(kb_f32_1(fa));
        for (int j = 0; j < NF + 3; j++) {
            double b = pool[j];
            mix(c_f64_lt(a, b)), mix(c_f64_le(a, b)), mix(c_f64_gt(a, b)), mix(c_f64_ge(a, b)), mix(c_f64_eq(a, b)), mix(c_f64_ne(a, b));
            mix(b_f64(a, b));
            float fb = (float)b;
            mix(c_f32_lt(fa, fb)), mix(c_f32_le(fa, fb)), mix(c_f32_gt(fa, fb)), mix(c_f32_ge(fa, fb)), mix(c_f32_eq(fa, fb)), mix(c_f32_ne(fa, fb));
            mix(b_f32(fa, fb));
        }
    }
    CHECK("cmpf");
}

// ---------------------------------------------------------------- extensions / narrow memory
static int8_t m8[16] = {0, 1, -1, 127, -128, 100, -100, 5, 6, 7, 8, 9, 10, 11, 12, -13};
static uint8_t mu8[16] = {0, 1, 255, 127, 128, 100, 200, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static int16_t m16[16] = {0, 1, -1, 32767, -32768, 1000, -1000, 5, 6, 7, 8, 9, 10, 11, 12, -13};
static uint16_t mu16[16] = {0, 1, 65535, 32767, 32768, 1000, 60000, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static int32_t m32[16] = {0, 1, -1, 0x7fffffff, (int32_t)0x80000000, 100000, -100000, 5, 6, 7, 8, 9, 10, 11, 12, -13};
static uint32_t mu32[16] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 100000, 3000000000u, 5, 6, 7, 8, 9, 10, 11, 12, 13};
NOINL static int32_t ld8s(int i) { return m8[i & 15]; }
NOINL static int32_t ld8u(int i) { return mu8[i & 15]; }
NOINL static int32_t ld16s(int i) { return m16[i & 15]; }
NOINL static int32_t ld16u(int i) { return mu16[i & 15]; }
NOINL static int64_t ld8sl(int i) { return m8[i & 15]; }
NOINL static int64_t ld8ul(int i) { return mu8[i & 15]; }
NOINL static int64_t ld16sl(int i) { return m16[i & 15]; }
NOINL static int64_t ld16ul(int i) { return mu16[i & 15]; }
NOINL static int64_t ld32sl(int i) { return m32[i & 15]; }
NOINL static int64_t ld32ul(int i) { return mu32[i & 15]; }
NOINL static int32_t trunc8(int32_t x) { return (int8_t)x; }
NOINL static int32_t trunc8u(int32_t x) { return (uint8_t)x; }
NOINL static int32_t trunc16(int32_t x) { return (int16_t)x; }
NOINL static int32_t trunc16u(int32_t x) { return (uint16_t)x; }
NOINL static int64_t sx32(int32_t x) { return x; }
NOINL static int64_t zx32(uint32_t x) { return x; }
NOINL static int32_t tr64(int64_t x) { return (int32_t)x; }
NOINL static int32_t sum8(int8_t a, int8_t b) { return (int8_t)(a + b); }
NOINL static uint32_t sum8u(uint8_t a, uint8_t b) { return (uint8_t)(a + b); }
NOINL static int32_t mul16(int16_t a, int16_t b) { return (int16_t)(a * b); }
NOINL static void st_narrow(int i, int64_t v) {
    static int8_t d8[16];
    static int16_t d16[16];
    static int32_t d32[16];
    d8[i & 15] = (int8_t)v, d16[i & 15] = (int16_t)v, d32[i & 15] = (int32_t)v;
    mix((uint64_t)(int64_t)d8[i & 15]), mix((uint64_t)(int64_t)d16[i & 15]), mix((uint64_t)(int64_t)d32[i & 15]);
    mix((uint8_t)d8[(i + 1) & 15]), mix((uint16_t)d16[(i + 1) & 15]), mix((uint32_t)d32[(i + 1) & 15]);
}

static void test_ext(void) {
    for (int i = 0; i < 16; i++) {
        mix((uint32_t)ld8s(i)), mix((uint32_t)ld8u(i)), mix((uint32_t)ld16s(i)), mix((uint32_t)ld16u(i));
        mix((uint64_t)ld8sl(i)), mix((uint64_t)ld8ul(i)), mix((uint64_t)ld16sl(i)), mix((uint64_t)ld16ul(i));
        mix((uint64_t)ld32sl(i)), mix((uint64_t)ld32ul(i));
    }
    for (int i = 0; i < N32; i++) {
        int32_t x = v32[i];
        mix((uint32_t)trunc8(x)), mix((uint32_t)trunc8u(x)), mix((uint32_t)trunc16(x)), mix((uint32_t)trunc16u(x));
        mix((uint64_t)sx32(x)), mix(zx32((uint32_t)x));
        for (int j = 0; j < N32; j++) {
            mix((uint32_t)sum8((int8_t)x, (int8_t)v32[j])), mix(sum8u((uint8_t)x, (uint8_t)v32[j]));
            mix((uint32_t)mul16((int16_t)x, (int16_t)v32[j]));
        }
    }
    for (int i = 0; i < N64; i++) {
        mix((uint32_t)tr64(v64[i]));
        st_narrow(i, v64[i]);
    }
    CHECK("ext");
}

// ---------------------------------------------------------------- floating point
#define FBIN(name, T, expr) NOINL static T name(T a, T b) { return expr; }
FBIN(fadd_d, double, a + b) FBIN(fsub_d, double, a - b) FBIN(fmul_d, double, a * b) FBIN(fdiv_d, double, a / b)
FBIN(fadd_f, float, a + b) FBIN(fsub_f, float, a - b) FBIN(fmul_f, float, a * b) FBIN(fdiv_f, float, a / b)
NOINL static double fneg_d(double a) { return -a; }
NOINL static float fneg_f(float a) { return -a; }
// FP constants of every encoding class (imm8-encodable, zero, general); separate statements with
// different operators so that no constant reassociation can change the rounding.
NOINL static double fcst_d(double a) {
    double s = a;
    s = s + 1.0, s = s * 0.5, s = s - 0.125, s = s / 31.0, s = s + 100.0, s = s * 16.0, s = s - 0.1, s = s + 2.0;
    s = s * 1e300, s = s - 0.0, s = s + 0.0;
    return s;
}
NOINL static float fcst_f(float a) {
    float s = a;
    s = s + 1.0f, s = s * 0.5f, s = s - 0.125f, s = s / 31.0f, s = s + 100.0f, s = s * 16.0f, s = s - 0.1f, s = s + 2.0f;
    s = s * 1e30f, s = s - 0.0f, s = s + 0.0f;
    return s;
}
NOINL static int32_t d2i(double a) { return (int32_t)a; }
NOINL static int64_t d2l(double a) { return (int64_t)a; }
NOINL static uint64_t d2ul(double a) { return (uint64_t)a; }
NOINL static uint32_t d2u(double a) { return (uint32_t)a; }
NOINL static int32_t f2i(float a) { return (int32_t)a; }
NOINL static int64_t f2l(float a) { return (int64_t)a; }
NOINL static uint64_t f2ul(float a) { return (uint64_t)a; }
NOINL static double i2d(int32_t a) { return a; }
NOINL static double l2d(int64_t a) { return (double)a; }
NOINL static double ul2d(uint64_t a) { return (double)a; }
NOINL static double u2d(uint32_t a) { return a; }
NOINL static float i2f(int32_t a) { return a; }
NOINL static float l2f(int64_t a) { return (float)a; }
NOINL static float ul2f(uint64_t a) { return (float)a; }
NOINL static float d2f(double a) { return (float)a; }
NOINL static double f2d(float a) { return a; }

static void test_fp(void) {
    for (int i = 0; i < NF; i++) {
        double a = vf[i];
        mixf(fneg_d(a)), mixf(fneg_f((float)a)), mixf(fcst_d(a)), mixf(fcst_f((float)a));
        mixf(d2f(a)), mixf(f2d((float)a));
        if (a > -2147483648.0 && a < 2147483647.0) mix((uint32_t)d2i(a)), mix((uint32_t)f2i((float)a / 2.0f));
        if (a > -9.2e18 && a < 9.2e18) mix((uint64_t)d2l(a)), mix((uint64_t)f2l((float)a));
        if (a >= 0 && a < 1.8e19) mix(d2ul(a)), mix(f2ul((float)a));
        if (a >= 0 && a < 4294967295.0) mix(d2u(a));
        for (int j = 0; j < NF; j++) {
            double b = vf[j];
            mixf(fadd_d(a, b)), mixf(fsub_d(a, b)), mixf(fmul_d(a, b)), mixf(fdiv_d(a, b));
            mixf(fadd_f((float)a, (float)b)), mixf(fsub_f((float)a, (float)b)), mixf(fmul_f((float)a, (float)b));
            mixf(fdiv_f((float)a, (float)b));
        }
    }
    for (int i = 0; i < N32; i++) mixf(i2d(v32[i])), mixf(u2d((uint32_t)v32[i])), mixf(i2f(v32[i]));
    for (int i = 0; i < N64; i++)
        mixf(l2d(v64[i])), mixf(ul2d((uint64_t)v64[i])), mixf(l2f(v64[i])), mixf(ul2f((uint64_t)v64[i]));
    CHECK("fp");
}

// ---------------------------------------------------------------- calls: spills, callee-saved, ABI
NOINL static int64_t ident64(int64_t x) { return x ^ 0x5a5a; }
NOINL static double identd(double x) { return x * 1.0; }
// 30 integer values and 20 doubles live across calls: more than the registers hold
NOINL static uint64_t pressure(int64_t a, double d) {
    int64_t x0 = a + 1, x1 = a * 3, x2 = a ^ 7, x3 = a - 9, x4 = a + 11, x5 = a * 5, x6 = a ^ 13, x7 = a - 17;
    int64_t x8 = a + 19, x9 = a * 7, x10 = a ^ 23, x11 = a - 29, x12 = a + 31, x13 = a * 9, x14 = a ^ 37, x15 = a - 41;
    int64_t x16 = a + 43, x17 = a * 11, x18 = a ^ 47, x19 = a - 53, x20 = a + 59, x21 = a * 13, x22 = a ^ 61, x23 = a - 67;
    int64_t x24 = a + 71, x25 = a * 15, x26 = a ^ 73, x27 = a - 79, x28 = a + 83, x29 = a * 17;
    double f0 = d + 1, f1 = d * 3, f2 = d - 7, f3 = d + 11, f4 = d * 5, f5 = d - 13, f6 = d + 17, f7 = d * 7, f8 = d - 19, f9 = d + 23;
    double f10 = d * 9, f11 = d - 29, f12 = d + 31, f13 = d * 11, f14 = d - 37, f15 = d + 41, f16 = d * 13, f17 = d - 43;
    double f18 = d + 47, f19 = d * 15;
    int64_t t = ident64(a);
    double u = identd(d);
    uint64_t r = (uint64_t)t;
    r = r * 31 + (uint64_t)(x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9);
    r = r * 31 + (uint64_t)(x10 + x11 + x12 + x13 + x14 + x15 + x16 + x17 + x18 + x19);
    r = r * 31 + (uint64_t)(x20 + x21 + x22 + x23 + x24 + x25 + x26 + x27 + x28 + x29);
    double s = u + f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12 + f13 + f14 + f15 + f16 + f17 + f18 + f19;
    mixf(s);
    return r + (uint64_t)(x0 ^ x29) + (uint64_t)(f0 * f19);
}
NOINL static int64_t many_i(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h,
                            int64_t i, int64_t j, int64_t k, int64_t l) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j + 11 * k + 12 * l;
}
NOINL static double many_d(double a, double b, double c, double d, double e, double f, double g, double h, double i,
                           double j, double k, double l) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j + 11 * k + 12 * l;
}
NOINL static double mixed(int a, double b, char c, float d, short e, int64_t f, double g, int h, float i, int j, double k,
                          int l, int64_t m, float n, double o, double p, double q, int r, char s, short t) {
    return a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p + q + r + s + t;
}
NOINL static int32_t narrow_args(int8_t a, uint8_t b, int16_t c, uint16_t d, int32_t e, uint32_t f, int8_t g, uint8_t h,
                                 int16_t i, uint16_t j, int8_t k, uint16_t l) {
    return a + b * 3 + c * 5 + d * 7 + e + (int32_t)f + g * 11 + h * 13 + i * 17 + j * 19 + k * 23 + l * 29;
}
NOINL static int fmt_check(void) {
    char buf[300];
    snprintf(buf, sizeof buf, "%d %lld %.3f %s %c %u %5.1f %lld %d %d %d %d %.2f %d %g", -5, 123456789012LL, 3.14159,
             "str", 'x', 4000000000u, 2.5, 9876543210LL, 1, 2, 3, 4, 99.5, 10, 0.25);
    uint64_t hh = 0;
    for (const char *p = buf; *p; p++) hh = hh * 131 + (unsigned char)*p;
    mix(hh);
    return (int)strlen(buf);
}
static uint64_t rec(int n, uint64_t a) { return n == 0 ? a : rec(n - 1, a * 6364136223846793005ULL + (uint64_t)n) ^ (a >> 3); }

static void test_calls(void) {
    for (int i = 0; i < N64; i++) {
        mix(pressure(v64[i], (double)(v64[i] % 1000)));
        mix((uint64_t)many_i(v64[i], 2, 3, 4, v64[(i + 1) % N64], 6, 7, 8, 9, v64[(i + 2) % N64], 11, 12));
        mixf(many_d((double)v64[i], 2.5, 3, 4, 5, 6, 7, 8, 9, 10, 11, (double)i));
        mixf(mixed((int)v64[i], 1.5, (char)i, 2.5f, (short)(i * 100), v64[(i + 3) % N64], 7.25, i, 8.5f, -i, 11.5, 12, i * (int64_t)13,
                   14.5f, 15.5, 16.5, 17.5, 18, 19, 20));
        mix(rec(i + 3, (uint64_t)v64[i]));
    }
    for (int i = 0; i < N32; i++) {
        int32_t x = v32[i];
        mix((uint32_t)narrow_args((int8_t)x, (uint8_t)x, (int16_t)x, (uint16_t)x, x, (uint32_t)x, (int8_t)(x >> 3), (uint8_t)(x >> 5),
                                  (int16_t)(x >> 7), (uint16_t)(x >> 9), (int8_t)(x >> 11), (uint16_t)(x >> 13)));
    }
    mix((uint64_t)fmt_check());
    CHECK("calls");
}

// ---------------------------------------------------------------- frames and displacements
typedef struct {
    char pad0[4100];
    int a;
    char pad1[32800];
    long b;
    double c;
    char tail[3];
} Big;
NOINL static uint64_t big_frame(int seed) {
    char small[7];
    int arr[9000];
    long long vl[40];
    Big big;
    memset(&big, 0, sizeof big);
    for (int i = 0; i < 9000; i += 97) arr[i] = seed + i;
    for (int i = 0; i < 40; i++) vl[i] = seed * (i + 1);
    big.a = seed;
    big.b = seed * 3L;
    big.c = seed * 0.5;
    big.tail[2] = (char)seed;
    small[6] = (char)(seed + 1);
    uint64_t r = 0;
    for (int i = 0; i < 9000; i += 97) r = r * 33 + (uint64_t)arr[i];
    for (int i = 0; i < 40; i++) r = r * 33 + (uint64_t)vl[i];
    r = r * 33 + (uint64_t)big.a + (uint64_t)big.b + (uint64_t)big.tail[2] + (uint64_t)small[6];
    mixf(big.c);
    return r;
}
NOINL static uint64_t disp_tests(const int32_t *p, const int64_t *q, const double *d, const int8_t *c, long i, long n) {
    uint64_t r = 0;
    r += (uint64_t)p[i] + (uint64_t)p[i + 1] + (uint64_t)p[i - 1] + (uint64_t)p[1024] + (uint64_t)p[1025];
    r += (uint64_t)q[i] + (uint64_t)q[i + 3] + (uint64_t)q[512] + (uint64_t)q[-2 + i] + (uint64_t)q[4096];
    r += (uint64_t)c[i] + (uint64_t)c[i + 1] + (uint64_t)c[4095] + (uint64_t)c[4096] + (uint64_t)c[-1 + i] + (uint64_t)c[n - 1];
    mixf(d[i] + d[i + 2] + d[511] + d[4097] + d[i - 1]);
    return r;
}
static int32_t big32[5000];
static int64_t big64[5000];
static double bigd[5000];
static int8_t bigc[20000];

static void test_frames(void) {
    for (int i = -3; i < 6; i++) mix(big_frame(i * 1001 + 7));
    for (int i = 0; i < 5000; i++) big32[i] = i * 7 - 3000, big64[i] = (int64_t)i * 1000003 - 99, bigd[i] = i * 0.25, bigc[i] = (int8_t)(i * 3);
    for (int i = 5000; i < 20000; i++) bigc[i] = (int8_t)(i % 100 - 50);
    for (long i = 1; i < 9; i++) mix(disp_tests(big32 + 8, big64 + 8, bigd + 8, bigc + 8, i, 17000));
    CHECK("frames");
}


// ---------------------------------------------------------------- aggregates by value
typedef struct { int a, b; } P8;
typedef struct { int64_t a, b, c; } P24;
typedef struct { int64_t a, b; } L2;
typedef struct { float x, y; } F2;
typedef struct { float a, b, c; } F3;
typedef struct { double x, y, z, w; } D4;
typedef struct { F2 p, q; } FF;
typedef struct { char c[3]; } C3;
typedef struct { int a; float f; } M8;
typedef struct { double d; int i; } DI;
typedef union { int64_t l; double d; } U8;
typedef union { char c[12]; int i[3]; } U12;
NOINL static int agg_p8(P8 p) { return p.a * 10 + p.b; }
NOINL static int64_t agg_p24(P24 p) { return p.a + 2 * p.b + 3 * p.c; }
NOINL static float agg_f2(F2 f) { return f.x * 2 + f.y; }
NOINL static double agg_d4(D4 d) { return d.x + 2 * d.y + 3 * d.z + 4 * d.w; }
NOINL static float agg_f3(F3 f) { return f.a + 2 * f.b + 3 * f.c; }
NOINL static float agg_ff(FF f) { return f.p.x + 2 * f.p.y + 3 * f.q.x + 4 * f.q.y; }
NOINL static int agg_c3(C3 c) { return c.c[0] + c.c[1] * 2 + c.c[2] * 3; }
NOINL static int agg_m8(M8 m) { return m.a + (int)m.f; }
NOINL static double agg_u8(U8 u) { return (double)u.l; }
NOINL static int agg_u12(U12 u) { return u.i[0] + u.i[1] * 3 + u.i[2] * 5; }
NOINL static P8 ret_p8(int a, int b) { P8 p = {a, b}; return p; }
NOINL static L2 ret_l2(int64_t a, int64_t b) { L2 p = {a, b}; return p; }
NOINL static P24 ret_p24(int64_t a) { P24 p = {a, a + 1, a + 2}; return p; }
NOINL static D4 ret_d4(double a) { D4 d = {a, a + 1, a + 2, a + 3}; return d; }
NOINL static F2 ret_f2(float a) { F2 f = {a, a * 2}; return f; }
NOINL static DI ret_di(double d, int i) { DI x = {d, i}; return x; }
NOINL static C3 ret_c3(int a) { C3 c = {{(char)a, (char)(a + 1), (char)(a + 2)}}; return c; }
NOINL static double agg_many(int a, F2 f, P8 p, D4 d, F3 g, int64_t l, P24 q, double e, int z) {
    return a + f.x + f.y + p.a + p.b + d.x + d.y + d.z + d.w + g.a + g.b + g.c + l + q.a + q.b + q.c + e + z;
}
NOINL static int64_t agg_stack(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h, P8 p, int i, P8 p2) {
    return a + b + c + d + e + f + g + h + p.a * 3 + p.b + i + p2.a * 5 + p2.b;
}
#ifdef IR_AGG_EXOTIC // placements the AST backend gets wrong or differently (overflowing HFAs and pointer-class aggregates on the stack)
NOINL static double agg_many14(int a, F2 f, P8 p, D4 d, F3 g, int64_t l, P24 q, double e, int z, D4 d2, P8 p2, D4 d3, C3 c3, double last) {
    return a + f.x + f.y + p.a + p.b + d.x + d.y + d.z + d.w + g.a + g.b + g.c + l + q.a + q.b + q.c + e + z + d2.x * 2 + d2.w +
        p2.a + p2.b * 3 + d3.y + d3.z * 4 + c3.c[2] + last;
}
NOINL static int64_t agg_stack_ptr(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h, P8 p, P24 q, L2 r, int i) {
    return a + b + c + d + e + f + g + h + p.a * 3 + p.b + q.a + q.b * 5 + q.c + r.a + r.b * 7 + i;
}
#endif
NOINL static P24 agg_chain(P24 q, int n) { return n == 0 ? q : agg_chain((P24){q.b, q.c, q.a + n}, n - 1); }
NOINL static double agg_roundtrip(D4 d) { D4 e = ret_d4(d.x + d.w); return agg_d4(e) + agg_d4(d); }

static void test_aggregates(void) {
    for (int i = 0; i < N32; i++) {
        int x = v32[i], y = v32[(i + 5) % N32];
        P8 p = {x, y};
        P24 q = {v64[i % N64], v64[(i + 1) % N64], v64[(i + 2) % N64]};
        F2 f = {(float)x / 7.0f, (float)y * 0.25f};
        F3 g = {f.x, f.y, f.x - f.y};
        D4 d = {(double)x, (double)y / 3.0, (double)q.a * 1e-10, (double)q.c};
        FF ff = {f, {f.y, f.x}};
        C3 c3 = {{(char)x, (char)y, (char)(x ^ y)}};
        M8 m = {x, (float)(y & 0xff)};
        U8 u8;
        u8.l = q.b;
        U12 u12;
        memcpy(&u12, &q, sizeof u12);
        mix((uint32_t)agg_p8(p)), mix((uint64_t)agg_p24(q)), mixf(agg_f2(f)), mixf(agg_d4(d)), mixf(agg_f3(g)), mixf(agg_ff(ff));
        mix((uint32_t)agg_c3(c3)), mix((uint32_t)agg_m8(m)), mixf(agg_u8(u8)), mix((uint32_t)agg_u12(u12));
        P8 rp = ret_p8(x, y);
        L2 rl = ret_l2(q.a, q.b);
        P24 r24 = ret_p24(q.c);
        D4 rd = ret_d4(d.x);
        F2 rf = ret_f2(f.x);
        DI rdi = ret_di(d.y, y);
        C3 rc = ret_c3(x);
        mix((uint32_t)rp.a), mix((uint32_t)rp.b), mix((uint64_t)rl.a), mix((uint64_t)rl.b), mix((uint64_t)r24.a), mix((uint64_t)r24.c);
        mixf(rd.x), mixf(rd.w), mixf(rf.x), mixf(rf.y), mixf(rdi.d), mix((uint32_t)rdi.i), mix((uint32_t)rc.c[0] + (uint32_t)rc.c[2]);
        mixf(agg_many(x, f, p, d, g, q.a, q, d.y, y));
        mix((uint64_t)agg_stack(1, 2, 3, 4, 5, 6, 7, 8, p, y, rp));
        P24 ch = agg_chain(q, 5);
        mix((uint64_t)ch.a + (uint64_t)ch.b + (uint64_t)ch.c);
        mixf(agg_roundtrip(d));
    }
    CHECK("aggregates");
#ifdef IR_AGG_EXOTIC
    for (int i = 0; i < N32; i++) {
        int x = v32[i], y = v32[(i + 5) % N32];
        P8 p = {x, y}, p2 = {y, x};
        P24 q = {v64[i % N64], v64[(i + 1) % N64], v64[(i + 2) % N64]};
        L2 rl = {q.a, q.c};
        F2 f = {(float)x / 7.0f, (float)y * 0.25f};
        F3 g = {f.x, f.y, f.x - f.y};
        D4 d = {(double)x, (double)y / 3.0, (double)q.a * 1e-10, (double)q.c};
        C3 c3 = {{(char)x, (char)y, (char)(x ^ y)}};
        mixf(agg_many14(x, f, p, d, g, q.a, q, d.y, y, d, p2, d, c3, f.x));
        mix((uint64_t)agg_stack_ptr(1, 2, 3, 4, 5, 6, 7, 8, p, q, rl, y));
    }
    CHECK("aggregates_exotic");
#endif
}

int main(void) {
    test_reg_ops();
    test_imm_ops();
    test_const_return();
    test_compares();
    test_ext();
    test_fp();
    test_calls();
    test_frames();
    test_aggregates();
#ifdef IR_ARITH_GEN
    return 0;
#else
#ifdef IR_AGG_EXOTIC
    int nsections = (int)(sizeof expected / sizeof expected[0]);
#else
    int nsections = (int)(sizeof expected / sizeof expected[0]) - 1; // the last entry is aggregates_exotic
#endif
    if (section_no != nsections) {
        printf("FAIL: %d sections run, table has %d\n", section_no, nsections);
        fails++;
    }
    if (!fails) printf("OK\n");
    return fails ? 1 : 0;
#endif
}
