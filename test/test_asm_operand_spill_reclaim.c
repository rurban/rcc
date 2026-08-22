#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Regression test for rcc x86-64 inline-asm codegen bug found via xz's
// LZMA range decoder (src/liblzma/rangecoder/range_decoder.h):
// rc_asm_bittree_n()'s single __asm__ has 8 simultaneously-live
// "=&r"/"+&r" register operands plus one "r" input -- 9 operands, one
// more than the 8-slot GP virtual-register pool. During operand setup,
// computing the "+&r" input-stream-pointer operand's store-back address
// while the pool is fully committed makes gen_addr() spill a still-live
// output's value register; free_reg() then restores that value but
// clears the register's used bit. The pre-drain address-restore pass
// hands the same physical register to the pointer's address, so the
// store-back drain pops that address into the register that still holds
// the output's asm result, clobbering it before it is written to
// memory. The decoder then silently produces wrong output (xz CLI:
// "Compressed data is corrupt" on valid streams).
//
// The test decodes the same LZMA bittree with the real inline-asm macro
// and with the plain-C reference macro, and checks the decoded symbol,
// the updated range/code state, and the updated probability array all
// match.

#if defined(__x86_64__) || defined(_M_X64)

#define RC_BIT_MODEL_TOTAL_BITS 11
#define RC_BIT_MODEL_TOTAL (UINT32_C(1) << RC_BIT_MODEL_TOTAL_BITS)
#define RC_BIT_MODEL_OFFSET ((UINT32_C(1) << RC_MOVE_BITS) - 1 - RC_BIT_MODEL_TOTAL)
#define RC_MOVE_BITS 5
#define RC_SHIFT_BITS 8
#define RC_TOP_VALUE (UINT32_C(1) << 24)

typedef uint16_t probability;

static struct {
    uint32_t range;
    uint32_t code;
} rc;
static uint8_t *rc_in_ptr;
static uint32_t rc_bound;
static uint32_t symbol;

// ---- plain-C reference (copied from xz range_decoder.h) ----
#define rc_normalize() \
do { \
    if (rc.range < RC_TOP_VALUE) { \
        rc.range <<= RC_SHIFT_BITS; \
        rc.code = (rc.code << RC_SHIFT_BITS) | *rc_in_ptr++; \
    } \
} while (0)

#define rc_if_0(prob) \
    rc_normalize(); \
    rc_bound = (rc.range >> RC_BIT_MODEL_TOTAL_BITS) * (prob); \
    if (rc.code < rc_bound)

#define rc_update_0(prob) \
do { \
    rc.range = rc_bound; \
    prob += (RC_BIT_MODEL_TOTAL - (prob)) >> RC_MOVE_BITS; \
} while (0)

#define rc_update_1(prob) \
do { \
    rc.range -= rc_bound; \
    rc.code -= rc_bound; \
    prob -= (prob) >> RC_MOVE_BITS; \
} while (0)

#define rc_bit_last(prob, action0, action1) \
do { \
    rc_if_0(prob) { \
        rc_update_0(prob); \
        action0; \
    } else { \
        rc_update_1(prob); \
        action1; \
    } \
} while (0)

#define rc_bit(prob, action0, action1) \
    rc_bit_last(prob, \
        symbol <<= 1; action0, \
        symbol = (symbol << 1) + 1; action1)

#define rc_bittree_bit(prob) rc_bit(prob, , )

#define c_rc_bittree3(probs, final_add) \
do { \
    symbol = 1; \
    rc_bittree_bit(probs[symbol]); \
    rc_bittree_bit(probs[symbol]); \
    rc_bittree_bit(probs[symbol]); \
    symbol += (uint32_t)(final_add); \
} while (0)

// ---- the real inline-asm macro (copied verbatim from xz) ----
#define rc_asm_y(str) str
#define rc_asm_n(str)

#define rc_asm_normalize \
    "cmp %[top_value], %[range]\n\t" \
    "jae 1f\n\t" \
    "shl %[shift_bits], %[code]\n\t" \
    "mov (%[in_ptr]), %b[code]\n\t" \
    "shl %[shift_bits], %[range]\n\t" \
    "inc %[in_ptr]\n" \
    "1:\n"

#define rc_asm_calc(prob) \
    "mov %[range], %[t0]\n\t" \
    "shr %[bit_model_total_bits], %[range]\n\t" \
    "imul %[" prob "], %[range]\n\t" \
    "sub %[range], %[t0]\n\t" \
    "mov %[code], %[t1]\n\t" \
    "sub %[range], %[code]\n\t"

#define rc_asm_bittree(a, b, first_only, middle_only, last_only) \
    first_only( \
        "movzwl 2(%[probs_base]), %[prob" #a "]\n\t" \
        "mov $2, %[symbol]\n\t" \
        "movzwl 4(%[probs_base]), %[prob" #b "]\n\t" \
    ) \
    middle_only( \
        "movzwl (%[probs_base], %q[symbol], 4), %[prob" #b "]\n\t" \
    ) \
    last_only( \
        "add %[symbol], %[symbol]\n\t" \
    ) \
        \
        rc_asm_normalize \
        rc_asm_calc("prob" #a) \
        \
        "cmovae %[t0], %[range]\n\t" \
        \
    first_only( \
        "movzwl 6(%[probs_base]), %[t0]\n\t" \
        "cmovae %[t0], %[prob" #b "]\n\t" \
    ) \
    middle_only( \
        "movzwl 2(%[probs_base], %q[symbol], 4), %[t0]\n\t" \
        "lea (%q[symbol], %q[symbol]), %[symbol]\n\t" \
        "cmovae %[t0], %[prob" #b "]\n\t" \
    ) \
        \
        "lea %c[bit_model_offset](%q[prob" #a "]), %[t0]\n\t" \
        "cmovb %[t1], %[code]\n\t" \
        "mov %[symbol], %[t1]\n\t" \
        "cmovae %[prob" #a "], %[t0]\n\t" \
        \
    first_only( \
        "sbb $-1, %[symbol]\n\t" \
    ) \
    middle_only( \
        "sbb $-1, %[symbol]\n\t" \
    ) \
    last_only( \
        "sbb %[last_sbb], %[symbol]\n\t" \
    ) \
        \
        "shr %[move_bits], %[t0]\n\t" \
        "sub %[t0], %[prob" #a "]\n\t" \
        "mov %w[prob" #a "], (%[probs_base], %q[t1], 1)\n\t"

#define rc_asm_bittree_n(probs_base_var, final_add, asm_str) \
do { \
    uint32_t t0; \
    uint32_t t1; \
    uint32_t t_prob0; \
    uint32_t t_prob1; \
    __asm__( \
        asm_str \
        : \
        [range]     "+&r"(rc.range), \
        [code]      "+&r"(rc.code), \
        [t0]        "=&r"(t0), \
        [t1]        "=&r"(t1), \
        [prob0]     "=&r"(t_prob0), \
        [prob1]     "=&r"(t_prob1), \
        [symbol]    "=&r"(symbol), \
        [in_ptr]    "+&r"(rc_in_ptr) \
        : \
        [probs_base]           "r"(probs_base_var), \
        [last_sbb]             "n"(-1 - (final_add)), \
        [top_value]            "n"(RC_TOP_VALUE), \
        [shift_bits]           "n"(RC_SHIFT_BITS), \
        [bit_model_total_bits] "n"(RC_BIT_MODEL_TOTAL_BITS), \
        [bit_model_offset]     "n"(RC_BIT_MODEL_OFFSET), \
        [move_bits]            "n"(RC_MOVE_BITS) \
        : \
        "cc", "memory"); \
} while (0)

#define asm_rc_bittree3(probs_base_var, final_add) \
    rc_asm_bittree_n(probs_base_var, final_add, \
        rc_asm_bittree(0, 1, rc_asm_y, rc_asm_n, rc_asm_n) \
        rc_asm_bittree(1, 0, rc_asm_n, rc_asm_y, rc_asm_n) \
        rc_asm_bittree(0, 1, rc_asm_n, rc_asm_n, rc_asm_y) \
    )

int main(void) {
    uint8_t in[256];
    for (int i = 0; i < 256; i++)
        in[i] = (uint8_t)(i * 131 + 17);

    uint32_t range = UINT32_MAX;
    uint32_t code = 0;
    for (int i = 0; i < 5; i++)
        code = (code << 8) | in[i];

    probability probs_c[16];
    probability probs_asm[16];
    for (int i = 0; i < 16; i++)
        probs_c[i] = probs_asm[i] = RC_BIT_MODEL_OFFSET;

    for (int iter = 0; iter < 64; iter++) {
        uint32_t sym_c, sym_asm, range_c, range_asm, code_c, code_asm;
        uint8_t *ptr_c, *ptr_asm;

        rc.range = range; rc.code = code; rc_in_ptr = in + 5; symbol = 0;
        c_rc_bittree3(probs_c, 0);
        sym_c = symbol; range_c = rc.range; code_c = rc.code; ptr_c = rc_in_ptr;

        rc.range = range; rc.code = code; rc_in_ptr = in + 5; symbol = 0;
        asm_rc_bittree3(probs_asm, 0);
        sym_asm = symbol; range_asm = rc.range; code_asm = rc.code; ptr_asm = rc_in_ptr;

        if (sym_c != sym_asm || range_c != range_asm || code_c != code_asm ||
            ptr_c != ptr_asm || memcmp(probs_c, probs_asm, sizeof(probs_c)) != 0) {
            fprintf(stderr,
                    "FAIL iter=%d sym=%u/%u range=%u/%u code=%u/%u inptr=%td/%td\n",
                    iter, sym_c, sym_asm, range_c, range_asm, code_c, code_asm,
                    ptr_c - in, ptr_asm - in);
            return 1;
        }
        // Advance state like the real decoder does.
        range = range_asm;
        code = code_asm;
    }

    printf("OK\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
