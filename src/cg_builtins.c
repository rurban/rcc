// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rcc.h"
#include "codegen_asm.h"
// All function names come from tok->name which is str_interned by the lexer,
// so pointer comparison is valid after we intern these literals once.
#define _BI(s) keyword_interned(s, sizeof(s) - 1)

char *bi_bswap16, *bi_bswap32, *bi_bswap64;
char *bi_clz, *bi_clzl, *bi_clzll;
char *bi_ctz, *bi_ctzl, *bi_ctzll;
char *bi_popcount, *bi_popcountl, *bi_popcountll;
char *bi_parity, *bi_parityl, *bi_parityll;
char *bi_clrsb, *bi_clrsbl, *bi_clrsbll;
char *bi_ffs, *bi_ffsl, *bi_ffsll;
char *bi_prefetch, *bi_frame_address, *bi_return_address;
char *bi_setjmp, *bi_longjmp;
char *bi_signbit, *bi_signbitf, *bi_signbitl;
char *bi_isinf, *bi_isinff, *bi_isinfl;
char *bi_isfinite, *bi_isfinitef, *bi_isfinitel;
char *bi_isnormal, *bi_isnormalf, *bi_isnormall;
char *bi_fpclassify, *bi_fpclassifyf, *bi_fpclassifyl;
char *bi_copysign, *bi_copysignf, *bi_copysignl;
char *bi_fma, *bi_fmaf, *bi_fmal;
char *bi_abs, *bi_labs, *bi_llabs;
char *bi_add_overflow, *bi_sub_overflow;
char *bi_mul_overflow, *bi_mul_overflow_p;
char *bi_memset, *bi_memcpy, *bi_memcmp;
char *bi_strlen, *bi_strcmp, *bi_strchr;
char *bi_s_abs, *bi_s_labs, *bi_s_llabs;
char *bi_s_strlen, *bi_s_strcmp, *bi_s_strchr;
char *bi_s_printf, *bi_s_fprintf, *bi_s_vprintf, *bi_s_vfprintf;
char *bi_s_puts, *bi_s_fputs;
char *bi_s_sprintf, *bi_s_snprintf;
char *bi_s_scanf, *bi_s_fscanf, *bi_s_sscanf;
char *bi_s_alloca;
char *bi_chk_printf, *bi_chk_vprintf;
char *bi_chk_fprintf, *bi_chk_vfprintf;
char *bi_sqrtps, *bi_sqrtss, *bi_rsqrtps;
char *bi_s_memset, *bi_s_memcpy, *bi_s_memcmp;


void init_builtin_names(void) {
    static bool done = false;
    if (done) return;
    done = true;
    bi_bswap16 = _BI("__builtin_bswap16");
    bi_bswap32 = _BI("__builtin_bswap32");
    bi_bswap64 = _BI("__builtin_bswap64");
    bi_clz = _BI("__builtin_clz");
    bi_clzl = _BI("__builtin_clzl");
    bi_clzll = _BI("__builtin_clzll");
    bi_ctz = _BI("__builtin_ctz");
    bi_ctzl = _BI("__builtin_ctzl");
    bi_ctzll = _BI("__builtin_ctzll");
    bi_popcount = _BI("__builtin_popcount");
    bi_popcountl = _BI("__builtin_popcountl");
    bi_popcountll = _BI("__builtin_popcountll");
    bi_parity = _BI("__builtin_parity");
    bi_parityl = _BI("__builtin_parityl");
    bi_parityll = _BI("__builtin_parityll");
    bi_clrsb = _BI("__builtin_clrsb");
    bi_clrsbl = _BI("__builtin_clrsbl");
    bi_clrsbll = _BI("__builtin_clrsbll");
    bi_ffs = _BI("__builtin_ffs");
    bi_ffsl = _BI("__builtin_ffsl");
    bi_ffsll = _BI("__builtin_ffsll");
    bi_prefetch = _BI("__builtin_prefetch");
    bi_frame_address = _BI("__builtin_frame_address");
    bi_return_address = _BI("__builtin_return_address");
    bi_setjmp = _BI("__builtin_setjmp");
    bi_longjmp = _BI("__builtin_longjmp");
    bi_signbit = _BI("__builtin_signbit");
    bi_signbitf = _BI("__builtin_signbitf");
    bi_signbitl = _BI("__builtin_signbitl");
    bi_isinf = _BI("__builtin_isinf");
    bi_isinff = _BI("__builtin_isinff");
    bi_isinfl = _BI("__builtin_isinfl");
    bi_isfinite = _BI("__builtin_isfinite");
    bi_isfinitef = _BI("__builtin_isfinitef");
    bi_isfinitel = _BI("__builtin_isfinitel");
    bi_isnormal = _BI("__builtin_isnormal");
    bi_isnormalf = _BI("__builtin_isnormalf");
    bi_isnormall = _BI("__builtin_isnormall");
    bi_fpclassify = _BI("__builtin_fpclassify");
    bi_fpclassifyf = _BI("__builtin_fpclassifyf");
    bi_fpclassifyl = _BI("__builtin_fpclassifyl");
    bi_copysign = _BI("__builtin_copysign");
    bi_copysignf = _BI("__builtin_copysignf");
    bi_copysignl = _BI("__builtin_copysignl");
    bi_fma = str_intern("__builtin_fma", 13);
    bi_fmaf = str_intern("__builtin_fmaf", 14);
    bi_fmal = str_intern("__builtin_fmal", 14);
    bi_abs = _BI("__builtin_abs");
    bi_labs = _BI("__builtin_labs");
    bi_llabs = _BI("__builtin_llabs");
    bi_add_overflow = _BI("__builtin_add_overflow");
    bi_sub_overflow = _BI("__builtin_sub_overflow");
    bi_mul_overflow = _BI("__builtin_mul_overflow");
    bi_mul_overflow_p = _BI("__builtin_mul_overflow_p");
    bi_memset = _BI("__builtin_memset");
    bi_memcpy = _BI("__builtin_memcpy");
    bi_memcmp = _BI("__builtin_memcmp");
    bi_strlen = _BI("__builtin_strlen");
    bi_strcmp = _BI("__builtin_strcmp");
    bi_strchr = _BI("__builtin_strchr");
    bi_s_abs = _BI("abs");
    bi_s_labs = _BI("labs");
    bi_s_llabs = _BI("llabs");
    bi_s_memset = _BI("memset");
    bi_s_memcpy = _BI("memcpy");
    bi_s_memcmp = _BI("memcmp");
    bi_s_strlen = _BI("strlen");
    bi_s_strcmp = _BI("strcmp");
    bi_s_strchr = _BI("strchr");
    bi_s_printf = _BI("printf");
    bi_s_fprintf = _BI("fprintf");
    bi_s_vprintf = _BI("vprintf");
    bi_s_vfprintf = _BI("vfprintf");
    bi_s_puts = _BI("puts");
    bi_s_fputs = _BI("fputs");
    bi_s_sprintf = _BI("sprintf");
    bi_s_snprintf = _BI("snprintf");
    bi_s_scanf = _BI("scanf");
    bi_s_fscanf = _BI("fscanf");
    bi_s_sscanf = _BI("sscanf");
    bi_s_alloca = _BI("alloca");
    bi_chk_printf = _BI("__printf_chk");
    bi_chk_vprintf = _BI("__vprintf_chk");
    bi_chk_fprintf = _BI("__fprintf_chk");
    bi_chk_vfprintf = _BI("__vfprintf_chk");
    bi_sqrtps = _BI("__builtin_ia32_sqrtps");
    bi_sqrtss = _BI("__builtin_ia32_sqrtss");
    bi_rsqrtps = _BI("__builtin_ia32_rsqrtps");
}

// Returns VReg with result, or R_NONE if call_target is not a recognized builtin.
VReg gen_builtin_call(Node *node, const char *call_target, VReg (*arg_gen)(Node *)) {
    if (!call_target) return R_NONE;

    // Quick prefix check: all names here start with "__builtin_" or are
    // short aliases (abs/labs/llabs). Skip the whole block for everything else.
    bool maybe_builtin = (call_target[0] == '_' && call_target[1] == '_') ||
        call_target == bi_s_abs || call_target == bi_s_labs || call_target == bi_s_llabs;
    bool is_bswap16 = false, is_bswap32 = false, is_bswap64 = false;
    bool is_clz = false, is_clzl = false, is_clzll = false;
    bool is_ctz = false, is_ctzl = false, is_ctzll = false;
    bool is_popcnt = false, is_popcntl = false, is_popcntll = false;
    bool is_parity = false, is_parityl = false, is_parityll = false;
    bool is_clrsb = false, is_clrsbl = false, is_clrsbll = false;
    bool is_ffs = false, is_ffsl = false, is_ffsll = false;
    bool is_prefetch = false, is_frame_addr = false, is_ret_addr = false;
    bool is_setjmp = false, is_longjmp = false;
    bool is_signbit = false, is_isinf = false, is_copysign_builtin = false, is_fma_builtin = false, is_abs_builtin = false;
    bool is_isfinite = false;
    bool is_isnormal = false, is_fpclassify = false;
    bool is_add_overflow = false, is_sub_overflow = false;
    bool is_mul_overflow = false, is_mul_overflow_p = false;
    if (maybe_builtin) {
        is_bswap16 = call_target == bi_bswap16;
        is_bswap32 = call_target == bi_bswap32;
        is_bswap64 = call_target == bi_bswap64;
        is_clz = call_target == bi_clz;
        is_clzl = call_target == bi_clzl;
        is_clzll = call_target == bi_clzll;
        is_ctz = call_target == bi_ctz;
        is_ctzl = call_target == bi_ctzl;
        is_ctzll = call_target == bi_ctzll;
        is_popcnt = call_target == bi_popcount;
        is_popcntl = call_target == bi_popcountl;
        is_popcntll = call_target == bi_popcountll;
        is_parity = call_target == bi_parity;
        is_parityl = call_target == bi_parityl;
        is_parityll = call_target == bi_parityll;
        is_clrsb = call_target == bi_clrsb;
        is_clrsbl = call_target == bi_clrsbl;
        is_clrsbll = call_target == bi_clrsbll;
        is_ffs = call_target == bi_ffs;
        is_ffsl = call_target == bi_ffsl;
        is_ffsll = call_target == bi_ffsll;
        is_prefetch = call_target == bi_prefetch;
        is_frame_addr = call_target == bi_frame_address;
        is_ret_addr = call_target == bi_return_address;
        is_setjmp = call_target == bi_setjmp;
        is_longjmp = call_target == bi_longjmp;
        is_signbit = call_target == bi_signbit ||
            call_target == bi_signbitf ||
            call_target == bi_signbitl;
        is_isfinite = call_target == bi_isfinite ||
            call_target == bi_isfinitef ||
            call_target == bi_isfinitel;
        is_isnormal = call_target == bi_isnormal ||
            call_target == bi_isnormalf ||
            call_target == bi_isnormall;
        is_fpclassify = call_target == bi_fpclassify ||
            call_target == bi_fpclassifyf ||
            call_target == bi_fpclassifyl;
        is_isinf = call_target == bi_isinf ||
            call_target == bi_isinff ||
            call_target == bi_isinfl;
        is_copysign_builtin = call_target == bi_copysign ||
            call_target == bi_copysignf ||
            call_target == bi_copysignl;
        is_fma_builtin = call_target == bi_fma ||
            call_target == bi_fmaf ||
            call_target == bi_fmal;
        is_abs_builtin = call_target == bi_abs ||
            call_target == bi_labs ||
            call_target == bi_llabs ||
            call_target == bi_s_abs ||
            call_target == bi_s_labs ||
            call_target == bi_s_llabs;
        is_add_overflow = call_target == bi_add_overflow;
        is_sub_overflow = call_target == bi_sub_overflow;
        is_mul_overflow = call_target == bi_mul_overflow;
        is_mul_overflow_p = call_target == bi_mul_overflow_p;
    }

    if (is_bswap16 || is_bswap32 || is_bswap64) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
#ifdef ARCH_ARM64
            if (is_bswap16) {
                asm_rev16(cg_sec, r, 4); // rev16 wr, wr
                asm_and_imm(cg_sec, r, 4, 0xffff); // and wr, wr, #0xffff
            } else if (is_bswap32) {
                asm_rev(cg_sec, r, 4); // rev wr, wr
            } else {
                asm_rev(cg_sec, r, 8); // rev xr, xr
            }
#else
            if (is_bswap16) {
                asm_rol_imm(cg_sec, r, 2, 8); // rol $8, %rx16
                asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
            } else if (is_bswap32) {
                asm_bswap(cg_sec, r, 4); // bswap rr
            } else {
                asm_bswap(cg_sec, r, 8); // bswap rr
            }
#endif
            return r;
        }
    }

    if (is_clz || is_clzl || is_clzll) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
            VReg r2 = alloc_reg();
            bool is64 = is_clzll || (is_clzl && sizeof(long) == 8);
#ifdef ARCH_ARM64
            if (is64)
                asm_clz(cg_sec, r2, r, 8); // clz rr2
            else
                asm_clz(cg_sec, r2, r, 4); // clz rr2
#else
            if (is64) {
                asm_clz(cg_sec, r2, r, 8); // lzcnt r64, r2_64
            } else {
                asm_clz(cg_sec, r2, r, 4); // lzcnt r32, r2_32
            }
#endif
            free_reg(r);
            return r2;
        }
    }

    if (is_ctz || is_ctzl || is_ctzll) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
            VReg r2 = alloc_reg();
            bool is64 = is_ctzll || (is_ctzl && sizeof(long) == 8);
#ifdef ARCH_ARM64
            if (is64) {
                asm_rbit(cg_sec, r, r, 8); // rbit xr, xr
                asm_clz(cg_sec, r2, r, 8); // clz rr2
            } else {
                asm_rbit(cg_sec, r, r, 4); // rbit wr, wr
                asm_clz(cg_sec, r2, r, 4); // clz rr2
            }
#else
            if (is64) {
                asm_tzcnt(cg_sec, r2, r, 8); // tzcnt r64, r2_64
            } else {
                asm_tzcnt(cg_sec, r2, r, 4); // tzcnt r32, r2_32
            }
#endif
            free_reg(r);
            return r2;
        }
    }

    if (is_popcnt || is_popcntl || is_popcntll || is_parity || is_parityl || is_parityll) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
            VReg r2 = alloc_reg();
            bool is64 = is_popcntll || is_parityll ||
                ((is_popcntl || is_parityl) && sizeof(long) == 8);
#ifdef ARCH_ARM64
            // Software popcount via NEON cnt
            char *tmp = is64 ? "v30.8b" : "v30.8b";
            if (is64) {
                asm_fmov_gp_to_d30(cg_sec, r); // fmov d30, x{r}
            } else {
                asm_fmov_gp_to_s30(cg_sec, r); // fmov s30, w{r}
            }
            asm_neon_cnt_v30(cg_sec); // cnt v30.8b, v30.8b
            asm_neon_addv_b30(cg_sec); // addv b30, v30.8b
            asm_fmov_s30_to_gp(cg_sec, r2); // fmov w{r2}, s30
            asm_and_imm(cg_sec, r2, 4, 0xff); // and w{r2}, w{r2}, #0xff
            (void)tmp;
#else
            if (is64) {
                asm_popcnt(cg_sec, r2, r, 8); // popcnt r64, r2_64
            } else {
                asm_popcnt(cg_sec, r2, r, 4); // popcnt r32, r2_32
            }
#endif
            if (is_parity || is_parityl || is_parityll)
#ifdef ARCH_ARM64
                asm_and_imm(cg_sec, r2, 4, 1); // and w{r2}, w{r2}, #1
#else
                asm_and_imm(cg_sec, r2, 4, 1); // and $1, %r32
#endif
            free_reg(r);
            return r2;
        }
    }

    if (is_clrsb || is_clrsbl || is_clrsbll) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
            VReg r2 = alloc_reg();
            bool is64 = is_clrsbll || (is_clrsbl && sizeof(long) == 8);
#ifdef ARCH_ARM64
            if (is64) {
                asm_cls(cg_sec, r2, r, 8); // cls x{r2}, x{r}
            } else {
                asm_cls(cg_sec, r2, r, 4); // cls w{r2}, w{r}
            }
#else
            // clrsb(x) = (x>=0 ? clz(x) : clz(~x)) - 1
            int lbl = ++rcc_label_count;
            int r3 = alloc_reg();
            if (is64) {
                asm_mov_reg_reg(cg_sec, r3, r, 8); // mov r -> r3 (copy)
                asm_sar_imm(cg_sec, r3, 8, 63); // sar $63, r3_64 (r3 = sign)
                asm_xor_reg_reg(cg_sec, r, r3, 8); // r ^= r3 (flip bits if negative)
                asm_clz(cg_sec, r2, r, 8); // lzcnt r64, r2_64
                asm_dec(cg_sec, r2, 8); // dec rr2
            } else {
                asm_mov_reg_reg(cg_sec, r3, r, 4); // mov r -> r3 (copy)
                asm_sar_imm(cg_sec, r3, 4, 31); // sar $31, r3_32 (r3 = sign)
                asm_xor_reg_reg(cg_sec, r, r3, 4); // r ^= r3 (flip bits if negative)
                asm_clz(cg_sec, r2, r, 4); // lzcnt r32, r2_32
                asm_dec(cg_sec, r2, 4); // dec rr2
            }
            free_reg(r3);
            (void)lbl;
#endif
            free_reg(r);
            return r2;
        }
    }

    if (is_ffs || is_ffsl || is_ffsll) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
            VReg r2 = alloc_reg();
            int r3 = alloc_reg();
            bool is64 = is_ffsll || (is_ffsl && sizeof(long) == 8);
#ifdef ARCH_ARM64
            if (is64) {
                asm_rbit(cg_sec, r, r, 8); // rbit xr, xr
                asm_clz(cg_sec, r3, r, 8); // clz rr3
                asm_add_imm(cg_sec, r3, 8, 1); // add $1, rr3
                asm_cmp_zero(cg_sec, r, 8); // cmp $0, rr
                asm_csel_zero_if_eq(cg_sec, r2, r3, 8); // csel r2, xzr, r3, eq
            } else {
                asm_rbit(cg_sec, r, r, 4); // rbit wr, wr
                asm_clz(cg_sec, r3, r, 4); // clz rr3
                asm_add_imm(cg_sec, r3, 4, 1); // add $1, rr3
                asm_cmp_zero(cg_sec, r, 4); // cmp $0, rr
                asm_csel_zero_if_eq(cg_sec, r2, r3, 4); // csel r2, wzr, r3, eq
            }
#else
            if (is64) {
                asm_movq_zero(cg_sec, r2); // xor rr2, rr2
                asm_bsf(cg_sec, r3, r, 8); // bsf r64, r3_64
                asm_lea_disp1(cg_sec, r3, r3, 8); // leaq 1(r3_64), r3_64
                asm_cmov(cg_sec, r2, r3, 8, X86_NZ); // cmovnz r3_64, r2_64
            } else {
                asm_movl_zero(cg_sec, r2); // xor r2_32, r2_32
                asm_bsf(cg_sec, r3, r, 4); // bsf r32, r3_32
                asm_lea_disp1(cg_sec, r3, r3, 4); // leal 1(r3_32), r3_32
                asm_cmov(cg_sec, r2, r3, 4, X86_NZ); // cmovnz r3_32, r2_32
            }
#endif
            free_reg(r3);
            free_reg(r);
            return r2;
        }
    }

    if (is_prefetch) {
        Node *addr = node->args;
        int rw = 0, locality = 3;
        // Parse rw and locality from constant args
        if (addr && addr->next && addr->next->kind == ND_NUM)
            rw = (int)addr->next->val;
        if (addr && addr->next && addr->next->next && addr->next->next->kind == ND_NUM)
            locality = (int)addr->next->next->val;
        if (addr) {
            VReg r = arg_gen(addr);
#ifdef ARCH_ARM64
            {
                // prfm: pldl1keep=0, pldl1strm=1, pstl1keep=9, pstl1strm=13
                int prfop = 0; // pldl1keep
                if (rw == 1 && locality == 0) prfop = 13; // pstl1strm
                else if (rw == 1)
                    prfop = 9; // pstl1keep
                else if (locality == 0)
                    prfop = 1; // pldl1strm
                asm_prfm(cg_sec, r, prfop); // prfm hint, [x{r}]
            }
#else
            // x86: prefetchw for write, otherwise nta/t0/t1/t2 by locality
            {
                // hint: 0=prefetchnta, 2=prefetcht1, 1=prefetcht2, 3=prefetcht0, 4=prefetchw
                int hint_idx = (rw == 1) ? 4 : locality == 0 ? 0
                    : locality == 1                          ? 1
                    : locality == 2                          ? 2
                                                             : 3;
                asm_prefetch(cg_sec, r, hint_idx); // prefetchXX (%reg)
            }
#endif
            free_reg(r);
            // Evaluate remaining args for side effects (if any expressions)
            for (Node *a = addr->next; a; a = a->next) {
                if (a->kind != ND_NUM) {
                    int ar = arg_gen(a);
                    if (ar >= 0) free_reg(ar);
                }
            }
        }
        return 0; // handled, void
    }

    if (is_signbit) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r_arg = arg_gen(arg);
#ifdef ARCH_ARM64
            asm_shr_imm(cg_sec, r_arg, 8, 63); // shr $63, r_arg
            return r_arg;
#else
            VReg r = alloc_reg();
            asm_movq_r_xmm(cg_sec, 0, r_arg); // movq r_arg, %xmm0
            asm_movq_xmm_r(cg_sec, r, 0); // movq %xmm0, r
            asm_shr_imm(cg_sec, r, 8, 63); // shrq $63, rr
            free_reg(r_arg);
            return r;
#endif
        }
    }

    /* __builtin_isinf(x): true if exponent all-1s, mantissa 0.
         * On x86: clear sign bit, compare against the inf bit pattern. */
    if (is_isinf) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r_arg = arg_gen(arg);
            VReg r = alloc_reg();
            VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
            // rcc promotes all floats to doubles in GP registers;
            // use 64-bit operations regardless of the source type size
            asm_mov_reg_reg(cg_sec, r, r_arg, 8);
            emit_mov_imm64(REG(r_tmp), 0x7fffffffffffffffULL);
            asm_and_reg_reg(cg_sec, r, r_tmp, 8);
            emit_mov_imm64(REG(r_tmp), 0x7ff0000000000000ULL);
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
            asm_cset(cg_sec, r, ARM64_EQ);
#else
            // Same on x86_64: floats are stored as doubles in GP regs
            asm_mov_reg_reg(cg_sec, r, r_arg, 8); // movq r_arg, r
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x7fffffffffffffffULL); // movabsq $0x7fff..., r_tmp
            asm_and_reg_reg(cg_sec, r, r_tmp, 8); // andq r_tmp, r (clear sign)
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x7ff0000000000000ULL); // movabsq $0x7ff0..., r_tmp
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8); // cmpq r_tmp, r
            asm_setcc(cg_sec, X86_RAX, X86_E); // sete %al
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1); // movzbl %al, %er
#endif
            free_reg(r_tmp);
            free_reg(r_arg);
            return r;
        }
    }

    /* __builtin_isfinite(x): true if exponent != 0x7FF (not inf, not NaN).
         * Mask to exponent bits, compare: (x & 0x7FF0000000000000) != 0x7FF0000000000000. */
    if (is_isfinite) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r_arg = arg_gen(arg);
            VReg r = alloc_reg();
            VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
            asm_mov_reg_reg(cg_sec, r, r_arg, 8);
            emit_mov_imm64(REG(r_tmp), 0x7FF0000000000000ULL);
            asm_and_reg_reg(cg_sec, r, r_tmp, 8);
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
            asm_cset(cg_sec, r, ARM64_NE);
#else
            asm_mov_reg_reg(cg_sec, r, r_arg, 8);
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x7FF0000000000000ULL);
            asm_and_reg_reg(cg_sec, r, r_tmp, 8);
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
            asm_setcc(cg_sec, X86_RAX, X86_NE);
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
#endif
            free_reg(r_tmp);
            free_reg(r_arg);
            return r;
        }
    }
    /* __builtin_isnormal(x): true if exponent in [1, 0x7FE] (normal, not subnormal/zero/inf/nan).
         * Range check: ((x & 0x7FF0000000000000) - 0x0010000000000000) < 0x7FE0000000000000 */
    if (is_isnormal) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r_arg = arg_gen(arg);
            VReg r = alloc_reg();
            VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
            asm_mov_reg_reg(cg_sec, r, r_arg, 8);
            emit_mov_imm64(REG(r_tmp), 0x7FF0000000000000ULL);
            asm_and_reg_reg(cg_sec, r, r_tmp, 8);
            emit_mov_imm64(REG(r_tmp), 0x0010000000000000ULL);
            asm_sub_reg_reg(cg_sec, r, r_tmp, 8);
            emit_mov_imm64(REG(r_tmp), 0x7FE0000000000000ULL);
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
            asm_cset(cg_sec, r, ARM64_LO);
#else
            asm_mov_reg_reg(cg_sec, r, r_arg, 8);
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x7FF0000000000000ULL);
            asm_and_reg_reg(cg_sec, r, r_tmp, 8);
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x0010000000000000ULL);
            asm_sub_reg_reg(cg_sec, r, r_tmp, 8);
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x7FE0000000000000ULL);
            asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
            asm_setcc(cg_sec, X86_RAX, X86_B);
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
#endif
            free_reg(r_tmp);
            free_reg(r_arg);
            return r;
        }
    }

    /* __builtin_fpclassify(x): classify floating-point value.
         * Returns: FP_NAN=0, FP_INFINITE=1, FP_NORMAL=2, FP_SUBNORMAL=3, FP_ZERO=4 */
    if (is_fpclassify) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r_arg = arg_gen(arg);
            VReg r = alloc_reg();
            VReg r_tmp = alloc_reg();
            VReg r_zero = alloc_reg();
            static int fc_counter;
            int c = fc_counter++;

            asm_mov_imm(cg_sec, r_zero, 8, 0);

            // r_tmp = r_arg & 0x7FF0000000000000  (exponent field)
            asm_mov_reg_reg(cg_sec, r_tmp, r_arg, 8);
#ifdef ARCH_ARM64
            emit_mov_imm64(REG(r), 0x7FF0000000000000ULL);
#else
            asm_movabs_phy(cg_sec, REG(r), 0x7FF0000000000000ULL);
#endif
            asm_and_reg_reg(cg_sec, r_tmp, r, 8);

            // if exponent == 0 → zero or subnormal
            asm_test_reg_reg(cg_sec, r_tmp, r_tmp, 8);
#ifdef ARCH_ARM64
            size_t jz_zero = asm_jcc_label(cg_sec, ARM64_EQ);
#else
            size_t jz_zero = asm_jcc_label(cg_sec, X86_E);
#endif

            // if exponent == 0x7FF → inf or nan
#ifdef ARCH_ARM64
            emit_mov_imm64(REG(r), 0x7FF0000000000000ULL);
#else
            asm_movabs_phy(cg_sec, REG(r), 0x7FF0000000000000ULL);
#endif
            asm_cmp_reg_reg(cg_sec, r_tmp, r, 8);
#ifdef ARCH_ARM64
            size_t jz_infnan = asm_jcc_label(cg_sec, ARM64_EQ);
#else
            size_t jz_infnan = asm_jcc_label(cg_sec, X86_E);
#endif

            // normal
            asm_mov_imm(cg_sec, r, 4, 2); // FP_NORMAL
            size_t jmp_done = asm_jmp_label(cg_sec);

            asm_fixup_add(cg_sec, jz_zero, format(".L.fc_zero.%d", c), 1);
            cg_def_label(format(".L.fc_zero.%d", c));
#ifdef ARCH_ARM64
            emit_mov_imm64(REG(r), 0x000FFFFFFFFFFFFFULL);
#else
            asm_movabs_phy(cg_sec, REG(r), 0x000FFFFFFFFFFFFFULL);
#endif
            asm_and_reg_reg(cg_sec, r_tmp, r, 8);
            asm_cmp_reg_reg(cg_sec, r_tmp, r_zero, 8);
            asm_mov_imm(cg_sec, r, 4, 4); // FP_ZERO
#ifdef ARCH_ARM64
            size_t jz_zero_done = asm_jcc_label(cg_sec, ARM64_EQ);
#else
            size_t jz_zero_done = asm_jcc_label(cg_sec, X86_E);
#endif
            asm_mov_imm(cg_sec, r, 4, 3); // FP_SUBNORMAL
            size_t jmp_zero_done = asm_jmp_label(cg_sec);

            asm_fixup_add(cg_sec, jz_infnan, format(".L.fc_infnan.%d", c), 1);
            cg_def_label(format(".L.fc_infnan.%d", c));
            asm_mov_reg_reg(cg_sec, r_tmp, r_arg, 8);
#ifdef ARCH_ARM64
            emit_mov_imm64(REG(r), 0x000FFFFFFFFFFFFFULL);
#else
            asm_movabs_phy(cg_sec, REG(r), 0x000FFFFFFFFFFFFFULL);
#endif
            asm_and_reg_reg(cg_sec, r_tmp, r, 8);
            asm_cmp_reg_reg(cg_sec, r_tmp, r_zero, 8);
            asm_mov_imm(cg_sec, r, 4, 1); // FP_INFINITE
#ifdef ARCH_ARM64
            size_t jz_inf_done = asm_jcc_label(cg_sec, ARM64_EQ);
#else
            size_t jz_inf_done = asm_jcc_label(cg_sec, X86_E);
#endif
            asm_mov_imm(cg_sec, r, 4, 0); // FP_NAN
            size_t jmp_infnan_done = asm_jmp_label(cg_sec);

            asm_fixup_add(cg_sec, jmp_done, format(".L.fc_done.%d", c), 0);
            asm_fixup_add(cg_sec, jz_zero_done, format(".L.fc_done.%d", c), 1);
            asm_fixup_add(cg_sec, jmp_zero_done, format(".L.fc_done.%d", c), 0);
            asm_fixup_add(cg_sec, jz_inf_done, format(".L.fc_done.%d", c), 1);
            asm_fixup_add(cg_sec, jmp_infnan_done, format(".L.fc_done.%d", c), 0);
            cg_def_label(format(".L.fc_done.%d", c));

            free_reg(r_zero);
            free_reg(r_tmp);
            free_reg(r_arg);
            return r;
        }
    }

    if (is_copysign_builtin) {
        Node *arg1 = node->args;
        Node *arg2 = arg1 ? arg1->next : NULL;
        if (arg1 && arg2 && !arg2->next) {
            VReg r_x = arg_gen(arg1);
            VReg r_y = arg_gen(arg2);
            VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
            // sign_mask = 0x8000000000000000
            asm_mov_imm(cg_sec, r_tmp, 8, (int64_t)0x8000000000000000ULL); // mov r_tmp, #-9223372036854775808
            asm_and_reg_reg(cg_sec, r_y, r_tmp, 8); // and r_y, r_y, r_tmp
            asm_not(cg_sec, r_tmp, 8); // mvn r_tmp, r_tmp
            asm_and_reg_reg(cg_sec, r_x, r_tmp, 8); // and r_x, r_x, r_tmp
            asm_or_reg_reg(cg_sec, r_x, r_y, 8); // orr r_x, r_x, r_y
#else
            // sign_mask = 0x8000000000000000
            asm_movabs_phy(cg_sec, REG(r_tmp), 0x8000000000000000ULL); // movabsq $sign_mask, r_tmp
            asm_and_reg_reg(cg_sec, r_y, r_tmp, 8); // andq r_tmp, r_y (y_sign)
            asm_not(cg_sec, r_tmp, 8); // notq r_tmp (~sign_mask)
            asm_and_reg_reg(cg_sec, r_x, r_tmp, 8); // andq r_tmp, r_x (|x|)
            asm_or_reg_reg(cg_sec, r_x, r_y, 8); // orq r_y, r_x (|x|+sign(y))
#endif
            free_reg(r_tmp);
            free_reg(r_y);
            return r_x;
        }
    }

    if (is_abs_builtin) {
        Node *arg = node->args;
        if (arg && !arg->next) {
            VReg r = arg_gen(arg);
#ifdef ARCH_ARM64
            int arg_size = (arg->ty && !is_flonum(arg->ty)) ? arg->ty->size : 8;
            if (arg_size <= 4 && !(arg->ty && arg->ty->is_unsigned))
                asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
            asm_cmp_zero(cg_sec, r, 8); // cmp $0, rr
            asm_cneg_mi(cg_sec, r, 8); // cneg x{r}, x{r}, mi
            return r;
#else
            VReg r2 = alloc_reg();
            int arg_size = (arg->ty && !is_flonum(arg->ty)) ? arg->ty->size : 8;
            if (arg_size <= 4 && !(arg->ty && arg->ty->is_unsigned))
                asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
            asm_mov_reg_reg(cg_sec, r2, r, 8); // mov rr -> rr2 (copy for sign computation)
            asm_sar_imm(cg_sec, r2, 8, 63); // sarq $63, r2_64
            asm_xor_reg_reg(cg_sec, r, r2, 8); // xorq r2, r (r ^= sign)
            asm_sub_reg_reg(cg_sec, r, r2, 8); // subq r2, r
            free_reg(r2);
            return r;
#endif
        }
    }

    if (is_fma_builtin) {
        if (call_target == bi_fma) call_target = "fma";
        else if (call_target == bi_fmaf)
            call_target = "fmaf";
        else
            call_target = "fmal";
        maybe_builtin = false;
    }

    if (is_frame_addr) {
        Node *arg = node->args;
        int r = alloc_reg();
        int depth = (arg && arg->kind == ND_NUM) ? (int)arg->val : 0;
#ifdef ARCH_ARM64
        arm64_orr_reg(cg_sec, 1, REG(r), ARM64_XZR, CG_ARM_FP, ARM64_LSL, 0); // mov r, x29
        for (int i = 0; i < depth; i++)
            asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
#else
        x86_mov_rr(cg_sec, 8, REG(r), CG_X86_FP); // mov %rbp, r
        for (int i = 0; i < depth; i++)
            asm_mov_indir(cg_sec, r, 8); // mov (%rr), %rr
#endif
        return r;
    }
    if (is_ret_addr) {
        Node *arg = node->args;
        int r = alloc_reg();
        int depth = (arg && arg->kind == ND_NUM) ? (int)arg->val : 0;
        // Follow frame pointer chain to find the return address.
        // frame pointer → [fp] = saved fp, [fp+8] = return address
#ifdef ARCH_ARM64
        arm64_orr_reg(cg_sec, 1, REG(r), ARM64_XZR, CG_ARM_FP, ARM64_LSL, 0); // mov r, x29
        for (int i = 0; i < depth; i++)
            asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
        asm_ldr_reg_off(cg_sec, r, r, 8, 8); // ldr x{r}, [x{r}, #8]  (return addr)
#else
        x86_mov_rr(cg_sec, 8, REG(r), CG_X86_FP); // mov %rbp, r
        for (int i = 0; i < depth; i++)
            asm_mov_indir(cg_sec, r, 8); // mov (%rr), %rr
        asm_mov_indir_disp(cg_sec, r, 8, 8); // mov 8(%rr), %rr
#endif
        return r;
    }
    if (is_setjmp) {
        // Inline __builtin_setjmp: save fp, resume_addr, sp to buf; return 0 or 1 (longjmp)
        int rbuf = arg_gen(node->args);
        int c = ++rcc_label_count;
        int r = alloc_reg();
#ifdef ARCH_ARM64
        asm_str_fp_reg(cg_sec, rbuf); // str x29, [x{rbuf}]  (buf[0] = fp)
        asm_adr_x16_label(cg_sec, format(".L.setjmp.%d", c)); // adr x16, .L.setjmp.c (resume addr)
        asm_str_x16_reg_uoff(cg_sec, rbuf, 8); // str x16, [x{rbuf}, #8]   (buf[1] = resume addr)
        asm_mov_reg_sp(cg_sec, ARM64_X16); // mov x16, sp
        asm_str_x16_reg_uoff(cg_sec, rbuf, 16); // str x16, [x{rbuf}, #16]  (buf[2] = sp)
        asm_mov_imm(cg_sec, r, 4, 0); // mov r, #0  (setjmp returns 0 normally)
        {
            size_t jmp_off = asm_jmp_label(cg_sec); // b .L.setjmp_end.c (skip resume block)
            cg_def_label(format(".L.setjmp.%d", c)); // .L.setjmp.c: (longjmp jumps here)
            asm_mov_retval(cg_sec, r, 8); // mov r, x0  (longjmp return value)
            asm_fixup_add(cg_sec, jmp_off, format(".L.setjmp_end.%d", c), 0);
            cg_def_label(format(".L.setjmp_end.%d", c)); // end of setjmp inline
        }
#else
        // x86_64: buf[0]=rbp, buf[1]=resume addr, buf[2]=rsp
        x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 0), X86_RBP); // movq %rbp, (rbuf)
        asm_lea_rip_reg(cg_sec, r, format(".L.sjr.%d", c)); // leaq .L.sjr.c(%rip), r
        x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 8), REG(r)); // movq r, 8(rbuf)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 16), X86_RSP); // movq %rsp, 16(rbuf)
        x86_xor_rr(cg_sec, 4, X86_RAX, X86_RAX); // xorl %eax, %eax
        {
            size_t jmp_off = asm_jmp_label(cg_sec); // jmp .L.sja.c
            cg_def_label(format(".L.sjr.%d", c)); // .L.sjr.c: (longjmp lands here, val in %rax)
            asm_fixup_add(cg_sec, jmp_off, format(".L.sja.%d", c), 0);
            cg_def_label(format(".L.sja.%d", c)); // .L.sja.c:
        }
        x86_mov_rr(cg_sec, 8, REG(r), X86_RAX); // movq %rax, r
#endif
        free_reg(rbuf);
        return r;
    }
    if (is_longjmp) {
        // Inline __builtin_longjmp: restore fp, sp from buf; jump to resume addr with val
        int rbuf = arg_gen(node->args);
        int rval = arg_gen(node->args->next);
#ifdef ARCH_ARM64
        asm_ldr_fp_reg(cg_sec, rbuf); // ldr x29, [x{rbuf}]   (buf[0] → restore fp)
        asm_ldr_x16_reg_uoff(cg_sec, rbuf, 16); // ldr x16, [x{rbuf}, #16] (buf[2] = saved sp)
        asm_mov_sp_reg(cg_sec, ARM64_X16); // mov sp, x16
        asm_ldr_x16_reg_uoff(cg_sec, rbuf, 8); // ldr x16, [x{rbuf}, #8]  (buf[1] = resume addr)
        asm_mov_x0_reg(cg_sec, REG(rval)); // mov x0, x{rval}  (setjmp return value)
        arm64_br(cg_sec, ARM64_X16); // br x16
#else
        // x86_64: restore rbp, load rax (val) and resume addr, then restore rsp, jmp
        int rtmp = alloc_reg();
        x86_mov_rm(cg_sec, 8, X86_RBP, x86_mem(REG(rbuf), 0)); // movq (rbuf), %rbp
        x86_mov_rm(cg_sec, 8, REG(rtmp), x86_mem(REG(rbuf), 8)); // movq 8(rbuf), rtmp
        x86_mov_rr(cg_sec, 8, X86_RAX, REG(rval)); // movq rval, %rax
        x86_mov_rm(cg_sec, 8, X86_RSP, x86_mem(REG(rbuf), 16)); // movq 16(rbuf), %rsp
        asm_jmp_reg(cg_sec, rtmp); // jmp *rtmp
        free_reg(rtmp);
#endif
        free_reg(rbuf);
        return 0; // handled, void
    }
#ifdef ARCH_ARM64
    if (is_add_overflow || is_sub_overflow || is_mul_overflow || is_mul_overflow_p) {
        Node *arga = node->args;
        Node *argb = arga ? arga->next : NULL;
        Node *argres = argb ? argb->next : NULL;
        int ra = arg_gen(arga);
        int rb = arg_gen(argb);
        int sz = arga && arga->ty && arga->ty->size > 4 ? 8 : 4;
        // Result type determines overflow check signedness and store width
        int res_sz = sz;
        bool res_unsigned = arga && arga->ty && arga->ty->is_unsigned;
        if (argres && argres->ty && argres->ty->kind == TY_PTR && argres->ty->base) {
            res_sz = argres->ty->base->size > 4 ? 8 : 4;
            res_unsigned = argres->ty->base->is_unsigned;
        }
        int r_result = alloc_reg();
        if (is_add_overflow) {
            if (res_sz == sz) {
                asm_adds(cg_sec, ra, rb, sz); // adds ra, ra, rb
                asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CS : ARM64_VS); // cset r_result, cs/vs
            } else if (res_sz < sz) {
                asm_adds(cg_sec, ra, rb, sz); // adds ra, ra, rb
                if (res_unsigned) {
                    if (res_sz == 2) {
                        arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 16, 16); // ubfx w{r_result}, w{ra}, #16, #16
                        asm_cmp_zero(cg_sec, r_result, 4); // cmp w{r_result}, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    } else if (res_sz == 1) {
                        arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 8, 24); // ubfx w{r_result}, w{ra}, #8, #24
                        asm_cmp_zero(cg_sec, r_result, 4); // cmp w{r_result}, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    } else {
                        asm_lsr_rd_rn_imm(cg_sec, r_result, ra, 8, (uint8_t)(res_sz * 8)); // lsr x{r_result}, x{ra}, #(res_sz*8)
                        asm_cmp_zero(cg_sec, r_result, 8); // cmp x{r_result}, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    }
                } else {
                    int rt = alloc_reg();
                    if (res_sz == 2) arm64_sxth(cg_sec, 0, REG(rt), REG(ra)); // sxth w{rt}, w{ra}
                    else
                        arm64_sxtb(cg_sec, 0, REG(rt), REG(ra)); // sxtb w{rt}, w{ra}
                    asm_cmp_reg_reg(cg_sec, rt, ra, sz); // cmp rt, ra
                    asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    free_reg(rt);
                }
            } else {
                // Widening: sign/zero-extend operands, do wider add
                if (sz == 4) {
                    if (arga->ty && arga->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(ra), REG(ra)); // uxtw x{ra}, w{ra}
                    else
                        asm_sxtw(cg_sec, REG(ra), REG(ra)); // sxtw x{ra}, w{ra}
                    if (argb->ty && argb->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(rb), REG(rb)); // uxtw x{rb}, w{rb}
                    else
                        asm_sxtw(cg_sec, REG(rb), REG(rb)); // sxtw x{rb}, w{rb}
                }
                asm_adds(cg_sec, ra, rb, 8); // adds ra, ra, rb
                if (res_unsigned && (!arga->ty || !arga->ty->is_unsigned || !argb->ty || !argb->ty->is_unsigned)) {
                    arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                    asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                } else {
                    asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CS : ARM64_VS); // cset r_result, cs/vs
                }
            }
        } else if (is_sub_overflow) {
            if (res_sz == sz) {
                asm_subs(cg_sec, ra, rb, sz); // subs ra, ra, rb
                asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CC : ARM64_VS); // cset r_result, cc/vs
            } else if (res_sz < sz) {
                asm_subs(cg_sec, ra, rb, sz); // subs ra, ra, rb
                if (res_unsigned) {
                    if (res_sz == 2) {
                        arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 16, 16); // ubfx w{r_result}, w{ra}, #16, #16
                        asm_cmp_zero(cg_sec, r_result, 4);
                        asm_cset(cg_sec, r_result, ARM64_NE);
                    } else if (res_sz == 1) {
                        arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 8, 24); // ubfx w{r_result}, w{ra}, #8, #24
                        asm_cmp_zero(cg_sec, r_result, 4);
                        asm_cset(cg_sec, r_result, ARM64_NE);
                    } else {
                        asm_lsr_rd_rn_imm(cg_sec, r_result, ra, 8, (uint8_t)(res_sz * 8)); // lsr x{r_result}, x{ra}, #(res_sz*8)
                        asm_cmp_zero(cg_sec, r_result, 8);
                        asm_cset(cg_sec, r_result, ARM64_NE);
                    }
                } else {
                    int rt = alloc_reg();
                    if (res_sz == 2) arm64_sxth(cg_sec, 0, REG(rt), REG(ra)); // sxth w{rt}, w{ra}
                    else
                        arm64_sxtb(cg_sec, 0, REG(rt), REG(ra)); // sxtb w{rt}, w{ra}
                    asm_cmp_reg_reg(cg_sec, rt, ra, sz); // cmp rt, ra
                    asm_cset(cg_sec, r_result, ARM64_NE);
                    free_reg(rt);
                }
            } else {
                // Widening
                if (sz == 4) {
                    if (arga->ty && arga->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(ra), REG(ra)); // uxtw x{ra}, w{ra}
                    else
                        asm_sxtw(cg_sec, REG(ra), REG(ra)); // sxtw x{ra}, w{ra}
                    if (argb->ty && argb->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(rb), REG(rb)); // uxtw x{rb}, w{rb}
                    else
                        asm_sxtw(cg_sec, REG(rb), REG(rb)); // sxtw x{rb}, w{rb}
                }
                asm_subs(cg_sec, ra, rb, 8); // subs ra, ra, rb
                if (res_unsigned && (!arga->ty || !arga->ty->is_unsigned || !argb->ty || !argb->ty->is_unsigned)) {
                    arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                    asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                } else {
                    asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CC : ARM64_VS); // cset r_result, cc/vs
                }
            }
        } else { // mul_overflow / mul_overflow_p
            // Multiply always produces double-width result (sz*2 bits).
            if (res_sz >= sz * 2) {
                // Result at least as wide as double-width product.
                bool a_signed = arga->ty && !arga->ty->is_unsigned;
                bool b_signed = argb->ty && !argb->ty->is_unsigned;
                if (sz == 8) asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                else
                    asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                if (res_unsigned && (a_signed || b_signed)) {
                    arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                    asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                } else {
                    asm_mov_imm(cg_sec, r_result, 4, 0); // mov w{r_result}, #0
                }
            } else if (res_sz == sz) {
                int r2 = alloc_reg();
                if (sz == 8) {
                    if (res_unsigned) {
                        asm_umulh(cg_sec, r2, ra, rb); // umulh r2, ra, rb
                        asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                        asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    } else {
                        int r3 = alloc_reg();
                        asm_smulh(cg_sec, r2, ra, rb); // smulh r2, ra, rb
                        asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                        asm_asr_rd_rn_imm(cg_sec, r3, ra, 8, 63); // asr r3, ra, #63
                        asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        free_reg(r3);
                    }
                } else {
                    if (res_unsigned) {
                        asm_umull(cg_sec, ra, ra, rb); // umull ra, wa, wb
                        asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, 32); // lsr r2, ra, #32
                        asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    } else {
                        // sxtw sign-extends the 32-bit result; compare to full 64-bit product
                        asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                        asm_sxtw(cg_sec, REG(r2), REG(ra)); // sxtw r2, wa
                        asm_cmp_reg_reg(cg_sec, r2, ra, 8); // cmp r2, ra
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    }
                }
                free_reg(r2);
            } else {
                // Narrowing: res_sz < sz.  Multiply, then range-check.
                int r2 = alloc_reg();
                if (sz == 8) {
                    asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                    asm_smulh(cg_sec, r2, ra, rb); // smulh r2, ra, rb
                } else {
                    asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                }
                if (res_unsigned) {
                    if (sz == 8) {
                        asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8)); // lsr r2, ra, #(res_sz*8)
                        asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                        arm64_csinc(cg_sec, 0, REG(r_result), REG(r_result), REG(r_result), ARM64_EQ); // cinc r_result, r_result, ne
                    } else {
                        asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8)); // lsr r2, ra, #(res_sz*8)
                        asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    }
                } else {
                    int r3 = alloc_reg();
                    if (sz == 8) {
                        asm_asr_rd_rn_imm(cg_sec, r3, ra, 8, (uint8_t)(res_sz * 8 - 1)); // asr r3, ra, #(res_sz*8-1)
                        asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        arm64_sbfx(cg_sec, 1, REG(r3), REG(ra), 0, res_sz * 8); // sbfx r3, ra, #0, #(res_sz*8)
                        asm_cmp_reg_reg(cg_sec, r3, ra, 8); // cmp r3, ra
                        arm64_csinc(cg_sec, 0, REG(r_result), REG(r_result), REG(r_result), ARM64_EQ); // cinc r_result, r_result, ne
                    } else {
                        asm_asr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8 - 1)); // asr r2, ra, #(res_sz*8-1)
                        asm_lsr_rd_rn_imm(cg_sec, r3, ra, 8, (uint8_t)(res_sz * 8)); // lsr r3, ra, #(res_sz*8)
                        asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                    }
                    free_reg(r3);
                }
                if (sz == 8)
                    free_reg(r2);
            }
        }
        if (argres && !is_mul_overflow_p) {
            VReg rr = arg_gen(argres);
            if (argres->ty && argres->ty->kind == TY_PTR && argres->ty->base &&
                argres->ty->base->kind == TY_INT128) {
                asm_str_reg_off(cg_sec, ra, rr, 8, 0);
                asm_sar_imm(cg_sec, ra, 8, 63);
                asm_str_reg_off(cg_sec, ra, rr, 8, 8); // str ra, [rr, #8]
            } else {
                asm_str_reg_off(cg_sec, ra, rr, res_sz, 0); // str reg(ra, res_sz), [rr]
            }
            free_reg(rr);
        }
        free_reg(ra);
        free_reg(rb);
        return r_result;
    }
#endif
#ifndef ARCH_ARM64
    if (is_add_overflow || is_sub_overflow || is_mul_overflow || is_mul_overflow_p) {
        Node *arga = node->args;
        Node *argb = arga ? arga->next : NULL;
        Node *argres = argb ? argb->next : NULL;
        int ra = arg_gen(arga);
        int rb = arg_gen(argb);
        // sz: operation size from the operands (at least 4).
        // sz_store: result pointer's pointed-to type (may differ).
        // sz_op: compute in max(sz, sz_store) so we can range-check the result.
        int sz = arga && arga->ty && arga->ty->size > 4 ? 8 : 4;
        int sz_store = sz;
        bool is_unsigned_store = arga && arga->ty && arga->ty->is_unsigned;
        // is_unsigned_op: true only when BOTH operands are unsigned (affects mul instruction
        // choice and widening strategy; mixed signed/unsigned must use signed arithmetic).
        bool ra_unsigned = arga && arga->ty && arga->ty->is_unsigned;
        bool rb_unsigned = argb && argb->ty && argb->ty->is_unsigned;
        bool is_unsigned_op = ra_unsigned && rb_unsigned;
        if (argres && argres->ty && argres->ty->kind == TY_PTR && argres->ty->base) {
            if (argres->ty->base->kind == TY_INT128) {
                // Treat as 8-byte for computation; store 128-bit separately below
                sz_store = 8;
                is_unsigned_store = argres->ty->base->is_unsigned;
            } else {
                sz_store = argres->ty->base->size > 4 ? 8 : 4;
                is_unsigned_store = argres->ty->base->is_unsigned;
            }
        }
        bool store_to_int128 = argres && argres->ty && argres->ty->kind == TY_PTR &&
            argres->ty->base && argres->ty->base->kind == TY_INT128;
        int sz_op = sz > sz_store ? sz : sz_store; // compute in larger size
        // If operands are narrower than sz_op, widen them per-operand signedness.
        if (sz_op > sz && sz == 4) {
            if (ra_unsigned)
                asm_movzx(cg_sec, ra, ra, 8, 4); // movl ra, ra (zero-ext)
            else
                asm_movsx(cg_sec, ra, ra, 8, 4);
            if (rb_unsigned)
                asm_mov_reg_reg(cg_sec, rb, rb, 4);
            else
                asm_movsx(cg_sec, rb, rb, 8, 4);
        }
        if (is_add_overflow || is_sub_overflow) {
            if (is_add_overflow)
                asm_add_reg_reg(cg_sec, ra, rb, sz_op); // add rb, ra
            else
                asm_sub_reg_reg(cg_sec, ra, rb, sz_op); // sub rb, ra
            // Detect overflow into sz_store bits.
            if (sz_op == sz_store && sz_op == sz) {
                // Same-size, same-type: use hardware flag directly.
                asm_setcc(cg_sec, X86_RAX, is_unsigned_store ? X86_C : X86_O); // setc/seto %al
            } else if (sz_op == sz_store && sz_op > sz) {
                // Widened signed ops into target type: check if mathematical
                // result fits.  For unsigned target a negative result overflows;
                // for signed target the 64-bit operation cannot itself overflow
                // (small inputs), so seto would always be 0 — use range check.
                if (is_unsigned_store) {
                    // negative result can't fit in unsigned
                    asm_setcc(cg_sec, X86_RAX, X86_S); // sets %al
                } else {
                    // signed: result fits in sz_op bits already; range-check to sz_store
                    x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx
                    x86_movsx(cg_sec, 8, 4, X86_RCX, X86_RCX); // movslq %ecx, %rcx
                    x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                }
            } else {
                // sz_op > sz_store: result is in full precision, range-check.
                if (is_unsigned_store) {
                    x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx (zero-ext)
                    // movl zero-extends implicitly; compare full result vs trunc
                    x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                } else {
                    x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx
                    x86_movsx(cg_sec, 8, 4, X86_RCX, X86_RCX); // movslq %ecx, %rcx
                    x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                }
            }
            if (argres) {
                int rr = arg_gen(argres);
                if (store_to_int128) {
                    asm_mov_reg_mem(cg_sec, ra, rr, 8); // movq ra, (rr)
                    x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                    x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 8), X86_RAX); // movq %rax, 8(rr)
                } else {
                    x86_mov_mr(cg_sec, sz_store, x86_mem(REG(rr), 0), REG(ra)); // mov ra, (rr)
                }
                free_reg(rr);
            }
        } else { // mul_overflow / mul_overflow_p
            int r2 = alloc_reg();
            if (sz_op == 8) {
                x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                if (is_unsigned_op && sz == sz_op) {
                    // Native unsigned 64-bit: mulq gives rdx:rax
                    asm_mul_1op(cg_sec, rb, 8); // mulq rb
                    x86_mov_rr(cg_sec, 8, REG(ra), X86_RAX); // movq %rax, ra
                    x86_mov_rr(cg_sec, 8, REG(r2), X86_RDX); // movq %rdx, r2
                    x86_test_rr(cg_sec, 8, X86_RDX, X86_RDX); // testq %rdx, %rdx
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                } else {
                    // Signed 64-bit (including sign-extended small operands):
                    // imulq gives rdx:rax; overflow if rdx != sign_ext(rax)
                    x86_imul_r(cg_sec, 8, REG(rb)); // imulq rb
                    x86_mov_rr(cg_sec, 8, REG(ra), X86_RAX); // movq %rax, ra
                    x86_mov_rr(cg_sec, 8, REG(r2), X86_RDX); // movq %rdx, r2
                    x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                    x86_cmp_rr(cg_sec, 8, X86_RDX, X86_RAX); // cmpq %rax, %rdx
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                    // For unsigned target with negative result: also signal overflow.
                    if (is_unsigned_store && !is_unsigned_op) {
                        // OR in the sign bit of the 64-bit result
                        x86_test_rr(cg_sec, 8, REG(ra), REG(ra)); // testq ra, ra
                        asm_setcc(cg_sec, X86_RCX, X86_S); // sets %cl
                        x86_or_rr(cg_sec, 1, X86_RAX, X86_RCX); // orb %cl, %al
                    }
                }
            } else {
                // 32-bit operands
                if (is_unsigned_op) {
                    x86_mov_rr(cg_sec, 4, X86_RAX, REG(ra)); // movl ra, %eax
                    asm_mul_1op(cg_sec, rb, 4); // mull rb
                    x86_mov_rr(cg_sec, 4, REG(ra), X86_RAX); // movl %eax, ra
                    x86_mov_rr(cg_sec, 4, REG(r2), X86_RDX); // movl %edx, r2
                    x86_test_rr(cg_sec, 4, X86_RDX, X86_RDX); // testl %edx, %edx
                    asm_setcc(cg_sec, X86_RAX, X86_NE);
                } else {
                    // Mixed or signed operands: extend each per its own signedness,
                    // then signed 64-bit multiply (exact for 32-bit inputs).
                    if (ra_unsigned)
                        x86_mov_rr(cg_sec, 4, X86_RAX, REG(ra)); // movl ra, %eax (zero-ext)
                    else
                        x86_movsx(cg_sec, 8, 4, X86_RAX, REG(ra)); // movslq ra, %rax
                    if (rb_unsigned)
                        x86_mov_rr(cg_sec, 4, X86_RCX, REG(rb)); // movl rb, %ecx (zero-ext)
                    else
                        x86_movsx(cg_sec, 8, 4, X86_RCX, REG(rb)); // movslq rb, %rcx
                    x86_imul_rr(cg_sec, 8, X86_RAX, X86_RCX); // imulq %rcx, %rax
                    x86_mov_rr(cg_sec, 4, REG(ra), X86_RAX); // movl %eax, ra
                    if (is_unsigned_store && sz_store == 8) {
                        // Negative result doesn't fit in unsigned 64-bit
                        x86_test_rr(cg_sec, 8, X86_RAX, X86_RAX); // testq %rax, %rax
                        asm_setcc(cg_sec, X86_RAX, X86_S); // sets %al
                        x86_movsx(cg_sec, 8, 4, REG(ra), REG(ra)); // movslq ra, ra
                    } else {
                        // Range check: sign-extend or zero-extend truncated result back
                        if (is_unsigned_store)
                            x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx (zero-ext)
                        else
                            x86_movsx(cg_sec, 8, 4, X86_RCX, REG(ra)); // movslq ra, %rcx
                        x86_cmp_rr(cg_sec, 8, X86_RCX, X86_RAX); // cmpq %rax, %rcx
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    }
                }
            }
            free_reg(r2);
            if (argres && !is_mul_overflow_p) {
                int rr = arg_gen(argres);
                if (store_to_int128) {
                    // Sign-extend 64-bit result to 128 bits and store
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 0), REG(ra)); // movq ra, (rr)
                    x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                    x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 8), X86_RAX); // movq %rax, 8(rr)
                } else {
                    x86_mov_mr(cg_sec, sz_store, x86_mem(REG(rr), 0), REG(ra)); // mov ra, (rr)
                }
                free_reg(rr);
            }
        }
        int r_result = alloc_reg();
        asm_movzx_phys(cg_sec, r_result, X86_RAX, 4, 1);
        free_reg(ra);
        free_reg(rb);
        return r_result;
    }
#endif

    return R_NONE;
}
