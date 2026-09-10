/* csmith 2295024 (fail_O1_optmismatch_8.c): rcc -O0/-O1/-O2 all computed
 * a wrong value for a volatile, `#pragma pack(1)`-packed 23-bit bitfield
 * compound-assign (`g_899.f8 &= (...)`), each giving a DIFFERENT garbage
 * result while gcc and clang (at both -O0 and -O2) agreed on 0.
 *
 * Root cause: in the bitfield ND_ASSIGN read-modify-write path,
 * `VReg r2 = gen(node->rhs);` is computed BEFORE `VReg ra =
 * gen_addr(node->lhs);`. Under register pressure, gen_addr()'s internal
 * alloc_reg() calls can spill r2's own physical register to make room
 * for computing the bitfield container's address, and hand that same
 * physical index back as `ra` -- silently aliasing r2 with ra. The
 * merge step's `asm_mov_reg_reg(rv, r2, 8)` then copied the container
 * ADDRESS (now sitting in r2's old register) into the new bitfield
 * value instead of the actual rhs, corrupting the stored field (and, in
 * a large enough surrounding expression, crashing when the garbage bit
 * pattern got used as a pointer elsewhere).
 *
 * Fixed by protect_from_addr_alias() in codegen.c: reload r2 from its
 * spill slot into a genuinely fresh register whenever gen_addr()'s
 * result aliases it, applied at all three `ra = gen_addr(node->lhs)`
 * sites in the bitfield assignment path (the shared rhs-reads-same
 * prelude, and the ARM64/x86 plain-assignment fallbacks).
 *
 * Deeply nested ternaries (matching codegen.c's own documented "each
 * nesting level's own r stacks up" register-pressure pattern) are used
 * here to force the alias under -O0, without needing -O1/-O2's
 * optimizer at all.
 */
#include <stdio.h>
#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct S0 {
    uint32_t f0;
    volatile int8_t f1;
    int32_t f2;
    int32_t f3;
    uint32_t f4;
    uint8_t f5;
    const uint8_t f6;
    int32_t f7;
    signed f8 : 23;
};
#pragma pack(pop)

static volatile struct S0 g_899 = {
    0xD0508574L, -1L, -1L, 0xE0A0D6ACL, 0xA8633CD3L, 0x60L, 0xE0L, 0x310EFFA0L, -2648};
static int64_t g_609f0 = -8;
static uint8_t l_1442[10] = {0, 0, 9, 1, 9, 0, 0, 9, 1, 9};
static volatile int32_t z = 0;

static int64_t safe_unary_minus_func_int64_t_s(int64_t x) { return -x; }

static int32_t f(void) {
    int32_t v = z ? 1
                   : (z ? 2
                        : (z ? 3
                             : (z ? 4
                                  : (z ? 5
                                       : (z ? 6
                                            : (z ? 7
                                                 : (g_899.f8 &=
                                                    (((safe_unary_minus_func_int64_t_s(g_609f0)),
                                                      l_1442[0]) == 255UL))))))));
    return v;
}

int main(void) {
    f();
    printf("g_899.f8=%d\n", (int)g_899.f8);
    return g_899.f8 == 0 ? 0 : 1;
}
