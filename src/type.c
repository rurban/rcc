// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"

// clang-format off
Type *ty_void    = &(Type){.kind=TY_VOID,    .size=1,  .align=1};
Type *ty_bool    = &(Type){.kind=TY_BOOL,    .size=1,  .align=1,  .is_unsigned=true};
#if defined(__aarch64__) && !defined(__APPLE__)
Type *ty_char    = &(Type){.kind=TY_CHAR,    .size=1,  .align=1,  .is_unsigned=true};
#else
Type *ty_char    = &(Type){.kind=TY_CHAR,    .size=1,  .align=1};
#endif
Type *ty_uchar   = &(Type){.kind=TY_CHAR,    .size=1,  .align=1,  .is_unsigned=true};
Type *ty_short   = &(Type){.kind=TY_SHORT,   .size=2,  .align=2};
Type *ty_ushort  = &(Type){.kind=TY_SHORT,   .size=2,  .align=2,  .is_unsigned=true};
Type *ty_int     = &(Type){.kind=TY_INT,     .size=4,  .align=4};
Type *ty_uint    = &(Type){.kind=TY_INT,     .size=4,  .align=4,  .is_unsigned=true};
#ifdef _WIN32
Type *ty_long    = &(Type){.kind=TY_LONG,    .size=4,  .align=4};
Type *ty_ulong   = &(Type){.kind=TY_LONG,    .size=4,  .align=4,  .is_unsigned=true};
#else
Type *ty_long    = &(Type){.kind=TY_LONG,    .size=8,  .align=8};
Type *ty_ulong   = &(Type){.kind=TY_LONG,    .size=8,  .align=8,  .is_unsigned=true};
#endif
Type *ty_llong   = &(Type){.kind=TY_LLONG,   .size=8,  .align=8};
Type *ty_ullong  = &(Type){.kind=TY_LLONG,   .size=8,  .align=8,  .is_unsigned=true};
Type *ty_int128  = &(Type){.kind=TY_INT128,  .size=16, .align=16};
Type *ty_uint128 = &(Type){.kind=TY_INT128,  .size=16, .align=16, .is_unsigned=true};

Type *size_t_type(void) {
#ifdef _WIN32
    return ty_ullong;
#else
    return ty_ulong;
#endif
}
Type *ty_float   = &(Type){.kind=TY_FLOAT,   .size=4,  .align=4};
Type *ty_double  = &(Type){.kind=TY_DOUBLE,  .size=8,  .align=8};
// IEEE 754-2008 decimal floating point (BID encoding): 7/16/34 digits.
Type *ty_decimal32  = &(Type){.kind=TY_DECIMAL32,  .size=4,  .align=4};
Type *ty_decimal64  = &(Type){.kind=TY_DECIMAL64,  .size=8,  .align=8};
Type *ty_decimal128 = &(Type){.kind=TY_DECIMAL128, .size=16, .align=16};
// Apple ARM64: long double is 64-bit (same as double).
// Linux ARM64/x86-64: long double is 128-bit (80-bit x87 padded, or IEEE quad).
#ifdef __APPLE__
Type *ty_ldouble = &(Type){.kind=TY_LDOUBLE, .size=8,  .align=8};
#else
Type *ty_ldouble = &(Type){.kind=TY_LDOUBLE, .size=16, .align=16};
#endif
Type *ty_nullptr_t = &(Type){.kind=TY_NULLPTR_T, .size=8, .align=8};
// clang-format on

bool is_integer(Type *ty) {
    return ty->kind == TY_BOOL || ty->kind == TY_CHAR || ty->kind == TY_SHORT ||
        ty->kind == TY_INT || ty->kind == TY_LONG || ty->kind == TY_LLONG ||
        ty->kind == TY_INT128 || ty->kind == TY_BITINT;
}

bool is_flonum(Type *ty) {
    return ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE || ty->kind == TY_LDOUBLE;
}

// IEEE 754-2008 decimal floating point. Values use the BID bit encoding
// and are passed in FP-class registers (SSE on x86-64, NEON s/d/q on
// arm64) per the platform decimal ABI, but ALL arithmetic is done by the
// bundled libdfp runtime calls (__bid_*3/__bid_*2), never by native FP
// instructions, so is_flonum() deliberately excludes them.
bool is_decimal(Type *ty) {
    return ty->kind == TY_DECIMAL32 || ty->kind == TY_DECIMAL64 || ty->kind == TY_DECIMAL128;
}

bool is_complex(Type *ty) {
    return ty && ty->kind == TY_COMPLEX;
}

Type *complex_type(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_COMPLEX;
    ty->base = base;
    ty->size = base->size * 2;
    ty->align = base->align;
    return ty;
}


bool is_number(Type *ty) {
    return is_integer(ty) || is_flonum(ty) || is_complex(ty) || is_decimal(ty);
}

Type *get_integer_type(int size, bool is_unsigned) {
    if (size <= 1)
        return is_unsigned ? ty_uchar : ty_char;
    if (size <= 2)
        return is_unsigned ? ty_ushort : ty_short;
    if (size <= 4)
        return is_unsigned ? ty_uint : ty_int;
    if (size <= 8)
        return is_unsigned ? ty_ullong : ty_llong;
    return is_unsigned ? ty_uint128 : ty_int128;
}

// C23 6.2.5p20 / 6.7.3.3: a _BitInt(N)'s storage follows the SysV x86-64
// psABI's _BitInt extension - N<=8/16/32/64 uses the same size/align as the
// equal-width plain integer type (so it participates in ordinary GP-register
// codegen unmodified); wider N is rounded up to a whole number of 8-byte
// "legs" (size = align = 8*ceil(N/64)), stored little-endian with the
// padding bits above N in the top leg left unspecified by the ABI (rcc
// always keeps them sign/zero-extended - see the wide-BitInt codegen
// helpers). Interned per (width, is_unsigned) pair so type_equal's pointer-
// identity fast path (and _Generic/typeof matching) sees two spellings of
// the same width - e.g. an explicit `_BitInt(7)` and a `typeof(42wb)` - as
// the identical Type.
#define BITINT_MAXWIDTH 512 // cache limit; wider types are still correctly
                            // constructed (see type_equal's TY_BITINT case),
                            // just allocated fresh instead of interned

// Set whenever a _BitInt(N) with N > 64 (size > 8, multi-limb) type is
// constructed in the current translation unit. The driver checks this after
// parse() and, when set, self-hosts the bitint runtime source (bitint_rt.c)
// into the same TU so the gen_bitint helper calls resolve. Reset at the
// start of each parse() (see parser.c).
bool parser_used_wide_bitint = false;
bool parser_used_decimal = false;

Type *bitint_type(int width, bool is_unsigned) {
    static Type *cache[2][BITINT_MAXWIDTH + 1];
    if (width < 1) width = 1;
    if (width > 64)
        parser_used_wide_bitint = true;
    Type **slot = (width <= BITINT_MAXWIDTH) ? &cache[is_unsigned ? 1 : 0][width] : NULL;
    if (slot && *slot) return *slot;
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_BITINT;
    ty->is_unsigned = is_unsigned;
    ty->bitint_width = width;
    if (width <= 8) {
        ty->size = 1;
        ty->align = 1;
    } else if (width <= 16) {
        ty->size = 2;
        ty->align = 2;
    } else if (width <= 32) {
        ty->size = 4;
        ty->align = 4;
    } else if (width <= 64) {
        ty->size = 8;
        ty->align = 8;
    } else {
        ty->size = 8 * ((width + 63) / 64);
        ty->align = 8;
    }
    if (slot) *slot = ty;
    return ty;
}

Type *pointer_to(Type *base) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->align = 8;
    ty->base = base;
    ty->is_unsigned = true; // pointers compare as unsigned
    return ty;
}

Type *array_of(Type *base, int64_t len) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->align = base->align;
    ty->base = base;
    return ty;
}

// Array-to-pointer decay (C11 6.3.2.1p3), preserving any const/volatile
// qualifier rcc stores on the ARRAY type itself (see
// parser.c's member_access_type()/qualify_type_copy() -- rcc represents
// e.g. a struct member array read through a const struct pointer as a
// qualified copy of the ARRAY type, not by qualifying its element type
// in place) onto the decayed pointer's pointee. Without this, `arr + i`/
// `arr[i]`/`&arr[i]` on such an array silently produced a plain,
// unqualified `char *` instead of `const char *`, e.g. postgres's
// `unconstify(char *, &sp->chars[i])` macro (sp: `const struct state *`)
// asserting `__builtin_types_compatible_p(__typeof(&sp->chars[i]), const
// char *)`, which real GCC accepts (sp->chars[i] is const through a
// const struct pointer) but rcc's decay silently dropped.
Type *decay_to_ptr(Type *arr_ty) {
    Type *elem = arr_ty->base;
    unsigned char missing = arr_ty->qual & (QUAL_CONST | QUAL_VOLATILE) & ~elem->qual;
    if (missing) elem = qualify_type_copy(elem, missing);
    return pointer_to(elem);
}

static Type *usual_arith_type(Type *lhs, Type *rhs);

static Node *new_scale_mul(Node *rhs, int size) {
    Node *num = arena_alloc(sizeof(Node));
    num->kind = ND_NUM;
    num->val = size;
    num->ty = ty_int;
    Node *node = arena_alloc(sizeof(Node));
    node->kind = ND_MUL;
    node->lhs = rhs;
    node->rhs = num;
    // The multiply's type must be the actual usual-arithmetic-conversion
    // result of `rhs * (int)size`, NOT a hardcoded int. Pointer arithmetic
    // `p + n` with a 64-bit n (e.g. `char *p + long n`) previously became
    // `p + (long)(n * 1)` where `n * 1` was typed int — codegen then
    // truncated n to 32 bits before the sign-extending cast, silently
    // corrupting every offset >= 2^31 (real bug: ksh93's
    // `printf -v v "%4000000000d"` produced a 32-bit-truncated 4 GB
    // padding, and segfaulted at exactly 2^32).
    node->ty = usual_arith_type(rhs->ty, num->ty);
    return node;
}

// Return a Node* computing the runtime size of a VLA type.
// For non-VLA types, returns a compile-time ND_NUM with size_t type.
// For VLA types, computes len * base_size recursively.
static Node *vla_size_node(Type *ty) {
    if (ty->kind != TY_VLA) {
        Node *num = arena_alloc(sizeof(Node));
        num->kind = ND_NUM;
        num->val = ty->size;
        num->ty = size_t_type();
        return num;
    }
    Node *len;
    if (ty->vla_len_expr) {
        len = ty->vla_len_expr;
    } else {
        len = arena_alloc(sizeof(Node));
        len->kind = ND_NUM;
        len->val = ty->array_len;
        len->ty = size_t_type();
    }
    Node *base = vla_size_node(ty->base);
    Node *node = arena_alloc(sizeof(Node));
    node->kind = ND_MUL;
    node->lhs = len;
    node->rhs = base;
    node->ty = size_t_type();
    return node;
}

static Type *integer_promotion(Type *ty) {
    if (!is_integer(ty))
        return ty;
    // C23 6.3.1.1p2: _BitInt types are never subject to the usual integer
    // promotions - a _BitInt(N) rvalue keeps its exact declared width and
    // signedness through every operation (unlike char/short, which widen to
    // int). Only the standard-rank types promote below int's width.
    if (ty->kind == TY_BITINT)
        return ty;
    if (ty->size < 4)
        return ty_int;
    return ty;
}

static Type *get_float_type(Type *lhs, Type *rhs) {
    if (lhs->kind == TY_LDOUBLE || rhs->kind == TY_LDOUBLE)
        return ty_ldouble;
    if (lhs->kind == TY_DOUBLE || rhs->kind == TY_DOUBLE)
        return ty_double;
    return ty_float;
}

// C23 6.3.1.8 (usual arithmetic conversions) for _Decimal32/64/128: the
// wider of the two decimal types wins; a binary floating type mixed with a
// decimal type converts to the decimal type.
static Type *get_decimal_type(Type *lhs, Type *rhs) {
    Type *d = NULL, *other = NULL;
    if (is_decimal(lhs)) {
        d = lhs;
        other = rhs;
    } else {
        d = rhs;
        other = lhs;
    }
    if (is_decimal(other)) {
        // Both decimal: wider wins (128 > 64 > 32).
        if (other->size > d->size) return other;
        return d;
    }
    // Decimal + binary float / integer: decimal wins. Integer operands are
    // converted to the decimal type (like float operands do to int).
    return d;
}

static int int_rank(Type *ty) {
    switch (ty->kind) {
    case TY_BOOL: return 0;
    case TY_CHAR: return 1;
    case TY_SHORT: return 2;
    case TY_INT: return 3;
    case TY_LONG: return 4;
    case TY_LLONG: return 5;
    case TY_INT128: return 6;
    // C23 6.3.1.1p1: a _BitInt(N)'s rank is strictly greater than that of
    // any standard integer type of lesser or equal width and strictly less
    // than any standard type wider than it - approximated here by ranking
    // purely on width, which places it correctly relative to every
    // standard type this compiler ever mixes it with (max standard width
    // is __int128 = 128 bits, well under this offset).
    case TY_BITINT: return 1000 + ty->bitint_width;
    default: return 3;
    }
}

static Type *usual_arith_type(Type *lhs, Type *rhs) {
    // Mixed scalar+complex: promote to complex
    if (is_complex(lhs) || is_complex(rhs)) {
        if (is_complex(lhs) && is_complex(rhs)) {
            // Both complex: promote to wider base type
            // If same type, return as-is (no promotion needed)
            if (lhs == rhs) return lhs;
            if (lhs->base == rhs->base && lhs->base->size == rhs->base->size &&
                lhs->base->is_unsigned == rhs->base->is_unsigned)
                return lhs;
            Type *common_base = usual_arith_type(lhs->base, rhs->base);
            if (common_base == lhs->base) return lhs;
            if (common_base == rhs->base) return rhs;
            return complex_type(common_base);
        }
        // One complex, one scalar (C11 6.3.1.8p1): if the scalar is a real
        // floating type whose rank exceeds the complex operand's base rank,
        // the complex operand converts to the scalar's type and the result
        // is complex with that (wider) base; otherwise the scalar converts
        // to the complex base and the complex type is kept. Integer scalars
        // always convert to the base (never widen it). Previously every
        // scalar promoted *to* the complex type, so `_Complex float * double`
        // stayed `_Complex float` and the wrongly-narrow result was then
        // mis-marshalled at call/return boundaries (mpc's mpc_get_dc
        // segfault: `I * mpfr_get_d(...)` stayed float complex).
        Type *cx = is_complex(lhs) ? lhs : rhs;
        Type *sc = is_complex(lhs) ? rhs : lhs;
        if (!is_number(sc)) return cx;
        if (is_flonum(sc) && cx->base && sc->size > cx->base->size)
            return complex_type(sc);
        return cx; // scalar promotes to complex
    }
    if (is_flonum(lhs) || is_flonum(rhs))
        return get_float_type(lhs, rhs);
    if (is_decimal(lhs) || is_decimal(rhs))
        return get_decimal_type(lhs, rhs);
    lhs = integer_promotion(lhs);
    rhs = integer_promotion(rhs);
    if (lhs->is_unsigned == rhs->is_unsigned)
        // Same signedness: pick the higher rank (equal rank => same type
        // already, either side works).
        return int_rank(lhs) >= int_rank(rhs) ? lhs : rhs;
    // Mixed signedness (C11 6.3.1.8p1, the three-way rule): identify the
    // unsigned and signed operand, then apply the rules in order.
    Type *uop = lhs->is_unsigned ? lhs : rhs;
    Type *sop = lhs->is_unsigned ? rhs : lhs;
    // Rule: if the unsigned operand's rank is >= the signed operand's,
    // the signed operand converts to the unsigned type (e.g. `int +
    // unsigned int`, or `long long + unsigned long long` -> unsigned).
    if (int_rank(uop) >= int_rank(sop))
        return uop;
    // Rule: otherwise, if the signed type can represent every value of
    // the unsigned type, the unsigned operand converts to the signed
    // type -- result stays SIGNED, unchanged. On the standard ladder
    // (char<short<int<long<long long, non-decreasing width at each
    // step) a signed operand of *strictly* greater rank is always
    // strictly wider, so it always qualifies (e.g. `long + unsigned
    // int`: long stays long, signed -- NOT promoted to unsigned long).
    // Only the same-width-different-rank pair (`long`/`long long` on
    // LP64, both 8 bytes) can fail this, handled by the final rule.
    if (sop->size > uop->size || sop->kind == TY_BITINT)
        return sop;
    // Rule: same width, signed cannot represent all unsigned values ->
    // convert both to the unsigned type CORRESPONDING TO the signed
    // operand's type (e.g. `long long + unsigned long` on LP64 ->
    // unsigned long long; `long + unsigned int` on LLP64, where `long`
    // and `int` are BOTH 4 bytes -> unsigned long, not unsigned int).
    // get_integer_type() picks purely by byte size and cannot
    // distinguish `long` from `int` at the same width (LLP64) or
    // `long` from `long long` at the same width (LP64) -- it always
    // prefers the long-long family for size 8 and the int family for
    // size 4, which happens to match when sop IS the long-long/int
    // member of that pair, but is WRONG when sop is `long` sharing that
    // width. Preserve sop's own kind explicitly for the two ambiguous
    // standard-ladder cases; fall back to get_integer_type() otherwise
    // (never ambiguous for char/short/int128, and a _BitInt sop is
    // handled by the branch above, never reaching here).
    if (sop->kind == TY_LONG)
        return ty_ulong;
    return get_integer_type(sop->size, true);
}

static void add_type_internal(Node *node);

// C: a null pointer constant is any integer constant expression with value 0
// (casts to integer types are allowed inside an ICE, so (char)0, (bool)0 and
// (enum e)0 all qualify), optionally cast to unqualified void*.
static bool is_null_pointer_constant(Node *n) {
    while (n && n->kind == ND_CAST && n->ty &&
           (is_integer(n->ty) ||
            (n->ty->kind == TY_PTR && n->ty->base &&
             n->ty->base->kind == TY_VOID && n->ty->base->qual == 0)))
        n = n->lhs;
    if (!n || !n->ty || !is_integer(n->ty))
        return false;
    if (n->kind == ND_NUM)
        return n->val == 0;
    // General case: any integer constant expression (not just a bare literal
    // after casts) evaluating to 0 also qualifies (C11 6.6p6). Needed for
    // e.g. linux/compiler.h's __is_constexpr(x) trick, which tests
    // "(void *)((long)(x) * 0l)" — an arithmetic ND_MUL, not a literal — for
    // being a null pointer constant precisely when x itself is compile-time
    // constant; eval_const_expr() correctly fails (non-constant) when x is
    // a runtime value, so the trick still resolves to "not a constant" then.
    long long v;
    return eval_const_expr(n, &v) && v == 0;
}

// Some __builtin_* functions are recognized by name in codegen and emit
// values of a specific width/signedness regardless of the (absent) prototype,
// which would otherwise default to plain `int`. Report their true return
// types here so that e.g. ND_RETURN doesn't mis-truncate/sign-extend a
// 64-bit __builtin_bswap64 result down to 32 bits.
static Type *builtin_return_type(const char *name);
static Type *ia32_builtin_ret(const char *fullname);

static Type *builtin_return_type(const char *name) {
    if (!name) return NULL;
    if (name == bi_bswap16) return ty_ushort;
    if (name == bi_bswap32) return ty_uint;
    if (name == bi_bswap64) return ty_ullong;
    // copysign builtins return double (handled inline in codegen, type must be correct)
    if (name == bi_copysign) return ty_double;
    if (name == bi_copysignf) return ty_float;
    if (name == bi_copysignl) return ty_ldouble;
    if (name == bi_unreachable) return ty_void;
    // __builtin_ia32_*: real GCC SIMD intrinsics. Their return types are
    // derived from the name's type-suffix family (see ia32_builtin_ret);
    // without this the calls would type as implicit int, and the headers'
    // `(__m128)__builtin_ia32_addps(...)` casts would become scalar-int ->
    // vector casts instead of vector -> vector bitcasts.
    if (strncmp(name, "__builtin_ia32_", 15) == 0) {
        Type *r = ia32_builtin_ret(name);
        return r;
    }
    return NULL;
}

// Cached vector types for __builtin_ia32_* return-type classification.
// kind: 0=float,1=double,2=signed char,3=short,4=int,5=long long.
// 8-byte (MMX), 16-byte (SSE) and 32-byte (AVX) shapes.
static Type *ia32_vec_ty(int kind, int bytes) {
    static Type *cache[6][3];
    int idx = bytes == 8 ? 0 : bytes == 32 ? 2
                                           : 1;
    if (!cache[kind][idx]) {
        Type *elem = kind == 0 ? ty_float
            : kind == 1        ? ty_double
            : kind == 2        ? ty_char
            : kind == 3        ? ty_short
            : kind == 4        ? ty_int
                               : ty_llong;
        cache[kind][idx] = rcc_make_vector_type(elem, bytes);
    }
    return cache[kind][idx];
}

static bool ia32_endswith(const char *n, const char *suf) {
    size_t nl = strlen(n), sl = strlen(suf);
    return nl >= sl && memcmp(n + nl - sl, suf, sl) == 0;
}

// Return type of a `__builtin_ia32_<name>` call, derived from the name.
// The SSE/SSE2/SSSE3/SSE4.1 intrinsic families encode their operand
// shape in the name suffix: ps/pd/ss/sd for float/double vectors,
// trailing b/w/d/q (+ optional "128") for integer vectors, and a
// handful of exact names for converts/moves/comparisons. The ARG types
// are always correct already (the headers cast every call site), so
// only the return type must be known to make `(__m128)call` a
// vector->vector bitcast instead of an implicit-int scalar cast.
static Type *ia32_builtin_ret(const char *fullname) {
    const char *n = fullname + 15; // "__builtin_ia32_"
    int L = (int)strlen(n);
    // Vector width encoded in the trailing "256"/"128" suffix (AVX/SSE).
    // 0 = no suffix: float families default to 16, int families to 8 (MMX).
    int vecsz = 0;
    if (ia32_endswith(n, "512_mask")) vecsz = 64; // AVX-512 masked forms
    else if (ia32_endswith(n, "512"))
        vecsz = 64;
    else if (ia32_endswith(n, "256"))
        vecsz = 32;
    else if (ia32_endswith(n, "128"))
        vecsz = 16;
    // AVX-512 result-width exceptions: pmovqd512_mask narrows 512->256,
    // extractf64x4_mask extracts 256, the compares return a 16-bit mask.
    if (strstr(n, "pmovqd512_mask"))
        return ia32_vec_ty(4, 32); // v8si
    if (strstr(n, "extractf64x4_mask"))
        return ia32_vec_ty(1, 32); // v4df
    if (strstr(n, "ucmpd512_mask") || strstr(n, "cmpd512_mask") ||
        strstr(n, "ucmpq512_mask") || strstr(n, "cmpq512_mask"))
        return ty_int; // __mmask16 result
    // shuf_i32x4_mask carries no 512 suffix in its name; without this it
    // types as implicit int and the header's `(__m512i)__builtin_ia32_...`
    // cast becomes a scalar->vector broadcast (the call's slot address got
    // splatted into the result instead of the shuffle result).
    if (!strcmp(n, "shuf_i32x4_mask"))
        return ia32_vec_ty(4, 64); // v16si
    if (!strcmp(n, "extractf64x4_mask"))
        return ia32_vec_ty(1, 32); // v4df (the name carries no 512 suffix)

    // --- exact scalar/void returns ---
    // int-returning: move-masks, compares-with-flags, CRC, string ops
    if (!strcmp(n, "movmskps") || !strcmp(n, "movmskpd") ||
        !strcmp(n, "pmovmskb") || !strcmp(n, "pmovmskb128") ||
        !strncmp(n, "comi", 4) || !strncmp(n, "ucomi", 5) ||
        !strncmp(n, "crc32", 5) || !strncmp(n, "ptest", 5) ||
        !strncmp(n, "pcmpestr", 8) || !strncmp(n, "pcmpistr", 8) ||
        !strcmp(n, "stmxcsr"))
        return ty_int;
    // long long: the 64-bit scalar-convert results
    if (!strcmp(n, "cvtss2si64") || !strcmp(n, "cvttss2si64") ||
        !strcmp(n, "cvtsd2si64") || !strcmp(n, "cvttsd2si64"))
        return ty_llong;
    // void-returning: fences, stores, cache/mxcsr control, MMX state
    if (!strcmp(n, "emms") || !strcmp(n, "femms") || !strcmp(n, "pause") ||
        !strcmp(n, "sfence") || !strcmp(n, "lfence") || !strcmp(n, "mfence") ||
        !strcmp(n, "clflush") || !strcmp(n, "prefetch") || !strcmp(n, "ldmxcsr") ||
        !strcmp(n, "monitor") || !strcmp(n, "mwait") ||
        !strncmp(n, "movnt", 5) || // movntps/pd/dq/nti/nti64/ntq
        !strcmp(n, "maskmovdqu") || !strcmp(n, "maskmovq") ||
        !strncmp(n, "storehps", 8) || !strncmp(n, "storelps", 8))
        return ty_void;

    // bf16 convert with mask: `__v8bf R = __builtin_ia32_cvtneps2bf16_v4sf_mask(...)`
    // must type as a 16-byte vector (16-bit bf16 elements), else the header's
    // scalar-init-of-vector parse fails. Checked BEFORE the generic cvt*
    // branch (which would classify it as int).
    if (!strcmp(n, "cvtneps2bf16_v4sf_mask"))
        return ia32_vec_ty(3, 16); // v8hi

    // F16C half-precision converts: vcvtph2ps/vcvtph2ps256 widen 16-bit
    // halfs to float (v4sf/v8sf, kind 0); vcvtps2ph/vcvtps2ph256 narrow
    // floats to packed 16-bit halfs, returned in a v8hi-shaped 128-bit
    // vector either way (256-bit input still packs down to 128 bits of
    // output). None of these start with "cvt" (the leading "v" of
    // "vcvtph2ps" defeats the memcmp(n,"cvt",3) check below) and none
    // end in a b/w/d/q lane letter, so without this they fell through
    // to the generic ty_int catch-all -- a `__v8hi H =
    // __builtin_ia32_vcvtps2ph(...)` initializer then hit a completely
    // unrelated codegen bug (a vector-typed declaration initialized
    // from an int-returning call whose own argument is itself a vector
    // mis-parsed as "expected an expression") instead of the real type
    // mismatch. Found via test_brotli's F16C-guarded encoder path.
    if (!strcmp(n, "vcvtph2ps")) return ia32_vec_ty(0, 16); // v4sf
    if (!strcmp(n, "vcvtph2ps256")) return ia32_vec_ty(0, 32); // v8sf
    if (!strcmp(n, "vcvtps2ph") || !strcmp(n, "vcvtps2ph256"))
        return ia32_vec_ty(3, 16); // v8hi

    // --- converts (cvt*) ---
    if (L > 3 && memcmp(n, "cvt", 3) == 0) {
        if (strstr(n, "2si")) return ty_int; // cvt*2si, cvt*2si64
        if (strstr(n, "2ps")) return ia32_vec_ty(0, 16); // cvtpi2ps, cvtdq2ps
        if (strstr(n, "2pd")) return ia32_vec_ty(1, 16); // cvtpi2pd, cvtdq2pd
        if (strstr(n, "2dq")) return ia32_vec_ty(4, 16); // cvtps2dq, cvttps2dq, cvtpd2dq, cvttpd2dq
        if (strstr(n, "2pi")) return ia32_vec_ty(4, 8); // cvtps2pi, cvttps2pi, cvtpd2pi, cvttpd2pi
        if (strstr(n, "2ss")) return ia32_vec_ty(0, 16); // cvtsi2ss, cvtsi642ss, cvtsd2ss
        if (strstr(n, "2sd")) return ia32_vec_ty(1, 16); // cvtsi2sd, cvtsi642sd, cvtss2sd
        return ty_int; // unknown cvt: assume int
    }

    // --- exact vector returns ---
    // bitwise int ops with no size letter in the name (MMX pand/por/pxor,
    // SSE2 pandn128): result is an 8/16-byte int vector matching the args
    if (!strcmp(n, "pand") || !strcmp(n, "por") || !strcmp(n, "pxor"))
        return ia32_vec_ty(4, 8); // v2si
    if (!strcmp(n, "pandn128"))
        return ia32_vec_ty(4, 16); // v4si
    if (!strcmp(n, "movq128") || !strcmp(n, "lddqu") || !strcmp(n, "movntdqa") ||
        !strcmp(n, "aesdec128") || !strcmp(n, "aesdeclast128") ||
        !strcmp(n, "aesenc128") || !strcmp(n, "aesenclast128") ||
        !strcmp(n, "aesimc128") || !strcmp(n, "aeskeygenassist128") ||
        !strcmp(n, "pclmulqdq128"))
        return ia32_vec_ty(5, 16); // v2di
    // vec_ext_v<TY>(v, i): return the ELEMENT scalar (the header casts
    // the result to the exact scalar type, but codegen needs the right
    // width: v2di elements are 8 bytes, v4sf elements are 4-byte float)
    if (!strncmp(n, "vec_ext_v2di", 12)) return ty_llong;
    if (!strncmp(n, "vec_ext_v4sf", 12)) return ty_float;
    if (!strncmp(n, "vec_ext_", 8)) return ty_int;
    if (!strcmp(n, "loadhps") || !strcmp(n, "loadlps") ||
        !strcmp(n, "movhlps") || !strcmp(n, "movlhps") ||
        !strcmp(n, "insertps128") || !strcmp(n, "dpps") ||
        !strcmp(n, "roundps") || !strcmp(n, "roundss") ||
        !strcmp(n, "blendps") || !strcmp(n, "blendvps"))
        return ia32_vec_ty(0, 16); // v4sf
    if (!strcmp(n, "dppd") || !strcmp(n, "roundpd") || !strcmp(n, "roundsd") ||
        !strcmp(n, "blendpd") || !strcmp(n, "blendvpd"))
        return ia32_vec_ty(1, 16); // v2df
    if (!strcmp(n, "pshufd")) return ia32_vec_ty(4, 16); // v4si
    if (!strcmp(n, "pshuflw") || !strcmp(n, "pshufhw")) return ia32_vec_ty(3, 16); // v8hi
    if (!strcmp(n, "pshufw")) return ia32_vec_ty(3, 8); // v4hi (MMX)
    if (!strcmp(n, "palignr")) return ia32_vec_ty(2, 8); // v8qi (MMX)
    if (!strcmp(n, "palignr128")) return ia32_vec_ty(2, 16); // v16qi
    if (!strcmp(n, "mpsadbw128") || !strcmp(n, "pblendvb128")) return ia32_vec_ty(2, 16);
    if (!strcmp(n, "phminposuw128")) return ia32_vec_ty(3, 16); // v8hi
    if (!strcmp(n, "pslldqi128") || !strcmp(n, "psrldqi128")) return ia32_vec_ty(5, 16); // v2di
    // 256->128 truncating "cast" intrinsics (_mm256_castX256_X128): the
    // trailing "256" here names the SOURCE width, not the result's -- the
    // generic float/int-family matchers below key off the SAME trailing
    // letters (ends_ps/ends_pd) or scan for a b/w/d/q lane letter, and
    // would otherwise size these as 32-byte (ps_ps256/pd_pd256, wrong
    // width) or fall through to plain `ty_int` (si_si256, not even a
    // vector) -- the latter made `__m128i x = _mm256_castsi256_si128(y);`
    // codegen a scalar-broadcast (movq+punpcklqdq) of the result slot's
    // own address into `x` instead of copying the low 128 bits.
    if (!strcmp(n, "ps_ps256")) return ia32_vec_ty(0, 16); // v4sf
    if (!strcmp(n, "pd_pd256")) return ia32_vec_ty(1, 16); // v2df
    if (!strcmp(n, "si_si256")) return ia32_vec_ty(5, 16); // v2di
    // vec_init_*/vec_set_*: return the named vector type. The suffix
    // starts right after "vec_init_v" (10 chars) / "vec_set_v" (9 chars).
    {
        const char *v = NULL;
        if (!strncmp(n, "vec_init_v", 10)) v = n + 10;
        else if (!strncmp(n, "vec_set_v", 9))
            v = n + 9;
        if (v) {
            if (v[1] == 's' && v[2] == 'f') return ia32_vec_ty(0, 16); // v4sf
            if (v[1] == 'd' && v[2] == 'i') return ia32_vec_ty(5, 16); // v2di
            if (v[0] == '8' && v[1] == 'q') return ia32_vec_ty(2, 8); // v8qi
            if (v[0] == '4' && v[1] == 'h') return ia32_vec_ty(3, 8); // v4hi
            if (v[0] == '2' && v[1] == 's') return ia32_vec_ty(4, 8); // v2si
            if (v[0] == '1' && v[1] == '6') return ia32_vec_ty(2, 16); // v16qi
            if (v[0] == '8' && v[1] == 'h') return ia32_vec_ty(3, 16); // v8hi
            if (v[0] == '4' && v[1] == 's') return ia32_vec_ty(4, 16); // v4si
            if (v[0] == '2' && v[1] == 'd') return ia32_vec_ty(5, 16); // v2di
        }
    }

    // int-returning 256-bit forms: the root (name without the 128/256
    // suffix) carries the exact-name meaning (movmskps256/movmskpd256
    // return a bitmask, pmovmskb256 the sign bits, vtest* the flags).
    {
        const char *base = n;
        int blen = L - (vecsz ? 3 : 0);
        if ((blen == 8 && memcmp(base, "movmskps", 8) == 0) ||
            (blen == 8 && memcmp(base, "movmskpd", 8) == 0) ||
            (blen == 9 && memcmp(base, "pmovmskb", 9) == 0) ||
            (blen >= 5 && memcmp(base, "vtest", 5) == 0))
            return ty_int;
    }

    // root-based vector results for names whose suffix carries no
    // b/w/d/q/ps/pd letter (palignr, permvarsi, vpermilps, movddup,
    // extract/insert, broadcasts).
    {
        const char *base = n;
        int blen = L - (vecsz ? (ia32_endswith(n, "512_mask") ? 7 : 3) : 0);
#define ROOT(s) (blen == (int)sizeof(s) - 1 && memcmp(base, s, sizeof(s) - 1) == 0)
        if (ROOT("palignr")) return ia32_vec_ty(2, vecsz ? vecsz : 8); // v32qi / v8qi
        if (ROOT("permvarsi")) return ia32_vec_ty(4, 32); // v8si
        if (ROOT("permvarsf")) return ia32_vec_ty(0, 32); // v8sf
        if (ROOT("vpermilvarps")) return ia32_vec_ty(0, 32); // v8sf
        if (ROOT("vpermilvarpd")) return ia32_vec_ty(1, 32); // v4df
        if (ROOT("vpermilps")) return ia32_vec_ty(0, 32); // v8sf (imm form)
        if (ROOT("vpermilpd")) return ia32_vec_ty(1, 32); // v4df (imm form)
        if (ROOT("permti")) return ia32_vec_ty(5, 32); // v4di (permute2x128_si256)
        if (ROOT("vperm2f128_si")) return ia32_vec_ty(5, 32); // v4di
        if (ROOT("vperm2f128_pd")) return ia32_vec_ty(1, 32); // v4df
        if (ROOT("vperm2f128_ps")) return ia32_vec_ty(0, 32); // v8sf
        if (ROOT("movddup") || ROOT("movsldup") || ROOT("movshdup"))
            return ia32_vec_ty(0, vecsz ? vecsz : 16); // v8sf / v4sf
        if (ROOT("vextractf128_pd")) return ia32_vec_ty(1, 16); // v2df
        if (ROOT("vextractf128_ps")) return ia32_vec_ty(0, 16); // v4sf
        if (ROOT("vextractf128_si") || ROOT("extract128i"))
            return ia32_vec_ty(4, 16); // v4si
        if (ROOT("vinsertf128_pd")) return ia32_vec_ty(1, 32); // v4df
        if (ROOT("vinsertf128_ps")) return ia32_vec_ty(0, 32); // v8sf
        if (ROOT("vinsertf128_si") || ROOT("insert128i"))
            return ia32_vec_ty(4, 32); // v8si
        if (ROOT("vbroadcastss") || ROOT("vbroadcastss_ps"))
            return ia32_vec_ty(0, 32); // v8sf
        if (ROOT("vbroadcastsd") || ROOT("vbroadcastsd_pd") || ROOT("vbroadcastsi"))
            return ia32_vec_ty(1, 32); // v4df
#undef ROOT
    }

    // --- float/double vector families by suffix ---
    // (exceptions handled by the exact matches above: movmskps/movmskpd,
    // movntps/movntpd, cvtss2si/cvtsd2si...)
    // The float/double families encode their width in the suffix that
    // PRECEDES the optional 128/256 size suffix (mulps256 -> mulps).
    {
        int blen = L - (vecsz ? (ia32_endswith(n, "512_mask") ? 7 : 3) : 0); // base name, no size suffix
        bool ends_ss = blen >= 2 && memcmp(n + blen - 2, "ss", 2) == 0;
        bool ends_ps = blen >= 2 && memcmp(n + blen - 2, "ps", 2) == 0;
        bool ends_sd = blen >= 2 && memcmp(n + blen - 2, "sd", 2) == 0;
        bool ends_pd = blen >= 2 && memcmp(n + blen - 2, "pd", 2) == 0;
        if (ends_ss || ends_ps)
            return ia32_vec_ty(0, vecsz ? vecsz : 16); // v4sf / v8sf
        if (ends_sd || ends_pd)
            return ia32_vec_ty(1, vecsz ? vecsz : 16); // v2df / v4df
    }

    // --- integer vector families: last b/w/d/q before trailing digits ---
    {
        // punpcklX*/punpckhX*: the element letter is the ONE AFTER the
        // l/h (punpcklbw interleaves BYTE lanes even though the mnemonic
        // ends in "w"); the backward scan below would pick the wrong one.
        if (!strncmp(n, "punpck", 6) && L >= 9) {
            char el = n[7]; // punpckl|h + element letter
            int kind = el == 'b' ? 2 : el == 'w' ? 3
                : el == 'd'                      ? 4
                                                 : 5;
            return ia32_vec_ty(kind, vecsz ? vecsz : 8);
        }
        // movshdup/movsldup: SSE3 packed-float unary shuffles
        if (!strcmp(n, "movshdup") || !strcmp(n, "movsldup"))
            return ia32_vec_ty(0, 16); // v4sf
        int end = vecsz ? L - (ia32_endswith(n, "512_mask") ? 7 : 3) : L;
        int kind = -1;
        for (int i = end - 1; i >= 0; i--) {
            if (n[i] == 'b') {
                kind = 2;
                break;
            } // byte lanes
            if (n[i] == 'w') {
                kind = 3;
                break;
            } // word lanes
            if (n[i] == 'd') {
                kind = 4;
                break;
            } // dword lanes
            if (n[i] == 'q') {
                kind = 5;
                break;
            } // qword lanes
        }
        if (kind >= 0)
            return ia32_vec_ty(kind, vecsz ? vecsz : 8);
    }
    return ty_int; // unknown: implicit int (better than a wrong vector type)
}

// Return type assumed for a function called WITHOUT a visible prototype.
// C's implicit declaration rule says such a call returns int, but the standard
// libc memory/string allocators return pointers. Assuming int for them produces
// spurious "pointer/integer mismatch" / truncated-pointer diagnostics when a
// header (or header shim) only declares them AFTER their first use — which
// happens e.g. when glibc's fortified <strings.h> defines bcopy() in terms of
// memmove() before <string.h>'s prototype is in scope. Only consulted on the
// implicit path; an explicit declaration always takes precedence.
static Type *implicit_return_type(const char *name) {
    if (!name) return ty_int;
    if (name == bi_s_alloca) return pointer_to(ty_void);
    static const char *ptr_funcs[] = {
        "memcpy",
        "memmove",
        "memset",
        "memchr",
        "strcpy",
        "strncpy",
        "strcat",
        "strncat",
        "strchr",
        "strrchr",
        "strstr",
        "strpbrk",
        "strdup",
        "strndup",
        "malloc",
        "calloc",
        "realloc",
        // Compiler intrinsics: always called without a declared prototype
        // (there is none to declare), so the implicit-int default is wrong
        // for these two the same way it's wrong for the libc allocators
        // above. Real kernel case: drivers/firmware/efi/runtime-wrappers.c's
        // "caller ?: __builtin_return_address(0)" — caller is a pointer,
        // and the ternary's branches must agree in type.
        "__builtin_return_address",
        "__builtin_frame_address",
        NULL,
    };
    for (int i = 0; ptr_funcs[i]; i++)
        if (strcmp(name, ptr_funcs[i]) == 0)
            return pointer_to(ty_void);
    return ty_int;
}

// Create composite type from two compatible types (for conditional expressions).
// Recursively merges pointer, array, function, and struct/union types.
static Type *composite_type(Type *t1, Type *t2) {
    if (!t1 || !t2 || t1 == t2)
        return t1;
    if (t1->kind != t2->kind)
        return t1;
    // codeql[cpp/long-switch]: central AST-node-kind dispatch; splitting cases into helpers is a large, purely-cosmetic refactor of core compiler internals, not attempted here.
    switch (t1->kind) {
    case TY_PTR:
        return pointer_to(composite_type(t1->base, t2->base));
    case TY_ARRAY:
        // Prefer the complete-sized array over incomplete
        if (t1->size == 0 && t2->size > 0) return t2;
        if (t2->size == 0 && t1->size > 0) return t1;
        return t1;
    case TY_FUNC: {
        Type *comp = arena_alloc(sizeof(Type));
        *comp = *t1;
        comp->return_ty = composite_type(t1->return_ty, t2->return_ty);
        // Prefer unspecified params (is_oldstyle) over specified
        if (t1->is_oldstyle && !t2->is_oldstyle) {
            comp->param_types = t1->param_types;
            comp->is_oldstyle = true;
        } else if (!t1->is_oldstyle && t2->is_oldstyle) {
            comp->param_types = t2->param_types;
            comp->is_oldstyle = true;
        }
        return comp;
    }
    case TY_STRUCT:
    case TY_UNION: {
        if (!t1->members || !t2->members) return t1;
        Member *m1 = t1->members, *m2 = t2->members;
        Member head = {}, *cur = &head;
        while (m1 && m2) {
            Member *cm = arena_alloc(sizeof(Member));
            *cm = *m1;
            if (m1->name && m2->name && strcmp(m1->name, m2->name) == 0)
                cm->ty = composite_type(m1->ty, m2->ty);
            cur->next = cm;
            cur = cm;
            m1 = m1->next;
            m2 = m2->next;
        }
        Type *result = arena_alloc(sizeof(Type));
        *result = *t1;
        result->members = head.next;
        // Recompute size/align for the composite struct
        if (result->kind == TY_STRUCT) {
            int64_t off = 0;
            int max_align = 1;
            for (Member *m = result->members; m; m = m->next) {
                if (m->bit_width > 0) {
                    // Bitfield: account for at least the storage unit size
                    int sz = m->ty ? m->ty->size : 4;
                    off += sz;
                    int al = m->ty ? m->ty->align : 4;
                    if (al > max_align) max_align = al;
                    continue;
                }
                if (!m->ty) continue;
                int al = m->ty->align;
                if (al > max_align) max_align = al;
                off = (off + al - 1) / al * al;
                m->offset = (int)off;
                off += m->ty->size;
            }
            result->size = (off + max_align - 1) / max_align * max_align;
            // Unnamed bitfields (padding) are absent from the member list but
            if (result->size < t1->size) result->size = t1->size;
            if (result->size < t2->size) result->size = t2->size;
            if (result->size == 0) result->size = t1->size > t2->size ? t1->size : t2->size;
            result->align = max_align;
        } else {
            int64_t max_sz = 0;
            int max_al = 1;
            for (Member *m = result->members; m; m = m->next) {
                if (m->ty && m->ty->size > max_sz) max_sz = m->ty->size;
                if (m->ty && m->ty->align > max_al) max_al = m->ty->align;
            }
            result->size = max_sz;
            result->align = max_al;
        }
        return result;
    }
    default:
        return t1;
    }
}

// Check if two types are structurally identical.
static bool same_type(Type *a, Type *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case TY_PTR:
        return same_type(a->base, b->base);
    case TY_ARRAY:
        return a->size == b->size && same_type(a->base, b->base);
    case TY_BITINT:
        return a->bitint_width == b->bitint_width && a->is_unsigned == b->is_unsigned;
    case TY_FUNC:
        if (a->is_variadic != b->is_variadic) return false;
        if (!same_type(a->return_ty, b->return_ty)) return false;
        // Compare parameter types
        {
            Type *pa = a->param_types, *pb = b->param_types;
            for (; pa && pb; pa = pa->param_next, pb = pb->param_next)
                if (!same_type(pa, pb)) return false;
            if (pa || pb) return false;
        }
        return true;
    default:
        return a->size == b->size && a->is_unsigned == b->is_unsigned;
    }
}

static void insert_arith_cast(Node **operand, Type *to) {
    Node *cast = arena_alloc(sizeof(Node));
    cast->kind = ND_CAST;
    cast->lhs = *operand;
    cast->ty = to;
    cast->tok = (*operand)->tok;
    cast->next = (*operand)->next;
    (*operand)->next = NULL;
    *operand = cast;
    add_type_internal(cast->lhs);
    add_type_internal(cast);
}

static void add_type_internal(Node *node) {
    if (!node || node->ty) return;

    add_type_internal(node->lhs);
    add_type_internal(node->rhs);
    add_type_internal(node->cond);
    add_type_internal(node->then);
    add_type_internal(node->els);
    add_type_internal(node->init);
    add_type_internal(node->inc);
    // case_next and default_case are internal control-flow chains for
    // codegen dispatch only; the body/next and lhs chains already cover
    // all case nodes.  Skipping them avoids cycles between lhs (forward
    // fall-through) and case_next (backward prepend) in large switches.
    // add_type_internal(node->case_next);
    // add_type_internal(node->default_case);

    for (Node *n = node->body; n; n = n->next)
        add_type_internal(n);
    for (Node *n = node->args; n; n = n->next)
        add_type_internal(n);

    // Atomic nodes: propagate types (no type computation needed here,
    // types are set by the parser in unary())
    switch (node->kind) {
    case ND_ATOMIC_LOAD:
    case ND_ATOMIC_STORE:
    case ND_ATOMIC_EXCHANGE:
    case ND_ATOMIC_CAS:
    case ND_ATOMIC_FENCE:
    case ND_ATOMIC_FETCH_OP:
        return;
    default:
        break;
    }

    // GCC __attribute__((vector_size)) element-wise operators. When an operand
    // is a vector, the result is the vector type and no scalar promotions or
    // casts are inserted — packed codegen (gen_vector) handles the lanes. A
    // vector combined with a scalar broadcasts the scalar to all lanes.
    {
        Type *lvt = node->lhs ? node->lhs->ty : NULL;
        Type *rvt = node->rhs ? node->rhs->ty : NULL;
        bool lv = lvt && lvt->is_vector;
        bool rv = rvt && rvt->is_vector;
        if (lv || rv) {
            switch (node->kind) {
            case ND_ADD:
            case ND_SUB:
            case ND_MUL:
            case ND_DIV:
            case ND_MOD:
            case ND_BITAND:
            case ND_BITOR:
            case ND_BITXOR:
            case ND_SHL: // lane-wise shifts (count vector or broadcast scalar)
            case ND_SHR:
            case ND_EQ: // vector comparisons yield a same-shape mask vector
            case ND_NE:
            case ND_LT:
            case ND_LE: {
                Type *vt = lv ? lvt : rvt;
                Type *elem = vt->base;
                node->ty = vt;
                if (!lv && node->lhs->ty != elem)
                    insert_arith_cast(&node->lhs, elem);
                if (!rv && node->rhs->ty != elem)
                    insert_arith_cast(&node->rhs, elem);
                return;
            }
            case ND_NEG:
            case ND_BITNOT:
                node->ty = lvt;
                return;
            default:
                break;
            }
        }
    }

    // codeql[cpp/long-switch]: central AST-node-kind dispatch; splitting cases into helpers is a large, purely-cosmetic refactor of core compiler internals, not attempted here.
    switch (node->kind) {
    case ND_ADD:
    case ND_SUB: {
        Type *lty = node->lhs->ty;
        Type *rty = node->rhs->ty;
        if (is_number(lty) && is_number(rty)) {
            node->ty = usual_arith_type(lty, rty);
            if (is_complex(node->ty)) {
                // Complex result: codegen's complex-arith handler copies the
                // operands component-wise with the RESULT's base size, so a
                // complex operand whose base differs (e.g. _Complex float
                // lhs of `I * b` after the usual arithmetic conversions
                // widened the result to _Complex double) must be cast to the
                // result type first; otherwise the handler over-reads the
                // smaller operand (mpc's mpc_get_dc segfault). Scalars stay
                // uncast — the handler converts them itself.
                if (is_complex(lty) && lty->base != node->ty->base)
                    insert_arith_cast(&node->lhs, node->ty);
                if (is_complex(rty) && rty->base != node->ty->base)
                    insert_arith_cast(&node->rhs, node->ty);
            } else if (is_flonum(node->ty) || is_decimal(node->ty)) {
                if (lty != node->ty)
                    insert_arith_cast(&node->lhs, node->ty);
                if (rty != node->ty)
                    insert_arith_cast(&node->rhs, node->ty);
            } else if (is_integer(node->ty)) {
                if (node->ty->size > lty->size)
                    insert_arith_cast(&node->lhs, node->ty);
                if (node->ty->size > rty->size)
                    insert_arith_cast(&node->rhs, node->ty);
            }
            return;
        }
        if (lty->base && is_integer(rty)) {
            if (lty->base->kind == TY_VLA) {
                Node *vla_sz = vla_size_node(lty->base);
                Node *mul = arena_alloc(sizeof(Node));
                mul->kind = ND_MUL;
                mul->lhs = node->rhs;
                mul->rhs = vla_sz;
                mul->ty = ty_ullong;
                node->rhs = mul;
            } else {
                node->rhs = new_scale_mul(node->rhs, lty->base->size);
            }
            if (node->rhs->ty->size < 8) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->rhs->ty->is_unsigned ? ty_ullong : ty_llong;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
            node->ty = (lty->kind == TY_ARRAY || lty->kind == TY_VLA) ? decay_to_ptr(lty) : lty;
            return;
        }
        if (is_integer(lty) && rty->base) {
            // ptr + int
            if (node->kind == ND_SUB) {
                // error: int - ptr is invalid
                node->ty = ty_int; // fallback
                return;
            }
            Node *tmp = node->lhs;
            node->lhs = node->rhs;
            node->rhs = tmp;
            if (rty->base->kind == TY_VLA) {
                Node *vla_sz = vla_size_node(rty->base);
                Node *mul = arena_alloc(sizeof(Node));
                mul->kind = ND_MUL;
                mul->lhs = node->rhs;
                mul->rhs = vla_sz;
                mul->ty = ty_ullong;
                node->rhs = mul;
            } else {
                node->rhs = new_scale_mul(node->rhs, rty->base->size);
            }
            if (node->rhs->ty->size < 8) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->rhs->ty->is_unsigned ? ty_ullong : ty_llong;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
            node->ty = (rty->kind == TY_ARRAY || rty->kind == TY_VLA) ? decay_to_ptr(rty) : rty;
            return;
        }
        if (lty->base && rty->base) {
            // ptr - ptr: the result is ptrdiff_t (64-bit on LP64), NOT int.
            // Typing it int made codegen compute the difference in 32 bits,
            // truncating any offset >= 2^31 before the element-size
            // division — real bug: sfio's `f->next - f->data` position
            // arithmetic in _sfexcept's string-buffer growth corrupted the
            // stream fields once the buffer passed 2 GB (ksh93's
            // `printf -v v "%4000000000d"` segfaulted at exactly 2^31).
            node->ty = ty_llong;
            return;
        }
        node->ty = ty_int;
        return;
    }
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITXOR:
    case ND_BITOR:
        node->ty = usual_arith_type(node->lhs->ty, node->rhs->ty);
        if (is_complex(node->ty)) {
            // See the ND_ADD/ND_SUB case above: cast complex operands whose
            // base differs from the result's base so codegen's complex-arith
            // handler reads correctly-sized components.
            if (is_complex(node->lhs->ty) && node->lhs->ty->base != node->ty->base)
                insert_arith_cast(&node->lhs, node->ty);
            if (is_complex(node->rhs->ty) && node->rhs->ty->base != node->ty->base)
                insert_arith_cast(&node->rhs, node->ty);
        } else if (is_flonum(node->ty)) {
            if (is_integer(node->lhs->ty))
                insert_arith_cast(&node->lhs, node->ty);
            if (is_integer(node->rhs->ty))
                insert_arith_cast(&node->rhs, node->ty);
        } else if (is_integer(node->ty)) {
            if (node->ty->size > node->lhs->ty->size)
                insert_arith_cast(&node->lhs, node->ty);
            if (node->ty->size > node->rhs->ty->size)
                insert_arith_cast(&node->rhs, node->ty);
        }
        return;
    case ND_NEG:
        node->ty = is_flonum(node->lhs->ty) ? node->lhs->ty : integer_promotion(node->lhs->ty);
        return;
    case ND_NOT:
        node->ty = ty_int;
        return;
    case ND_FNUM:
        return;
    case ND_REAL:
    case ND_IMAG:
        if (node->lhs->ty && node->lhs->ty->kind == TY_COMPLEX)
            node->ty = node->lhs->ty->base;
        else
            node->ty = node->lhs->ty ? node->lhs->ty : ty_int;
        return;
    case ND_SHL:
    case ND_SHR:
        node->ty = integer_promotion(node->lhs->ty);
        return;
    case ND_LOGAND:
    case ND_LOGOR:
        node->ty = ty_int;
        return;
    case ND_ASSIGN:
        if (node->lhs->ty && node->rhs->ty) {
            bool lf = is_flonum(node->lhs->ty);
            bool rf = is_flonum(node->rhs->ty);
            if (node->lhs->ty->kind == TY_BOOL && node->rhs->ty->kind != TY_BOOL) {
                // Storing into a _Bool must normalize to 0/1 (C11 6.3.1.2),
                // not truncate the low byte — 10 assigns as true, not 10.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if ((lf && !rf) || (!lf && rf) ||
                       (lf && rf && node->lhs->ty->size != node->rhs->ty->size) ||
                       (is_decimal(node->lhs->ty) && !is_decimal(node->rhs->ty)) ||
                       (is_decimal(node->lhs->ty) && is_decimal(node->rhs->ty) &&
                        node->lhs->ty->size != node->rhs->ty->size)) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!lf && !rf && is_integer(node->lhs->ty) && is_integer(node->rhs->ty) &&
                       node->lhs->ty->size > node->rhs->ty->size) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!lf && !rf && is_integer(node->lhs->ty) && is_integer(node->rhs->ty) &&
                       node->rhs->ty->kind == TY_INT128 && node->lhs->ty->kind != TY_INT128) {
                // Truncation from int128 to smaller: insert explicit cast so codegen
                // knows to extract the value from the 128-bit slot.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (is_complex(node->lhs->ty) && is_complex(node->rhs->ty) &&
                       node->lhs->ty->base && node->rhs->ty->base &&
                       (node->lhs->ty->base->size != node->rhs->ty->base->size ||
                        is_flonum(node->lhs->ty->base) != is_flonum(node->rhs->ty->base))) {
                // Complex-to-complex with a different base (e.g. assigning a
                // _Complex double to a _Complex long double): without the
                // cast the assign codegen copies node->lhs->ty->size raw
                // bytes from the smaller rhs slot, over-reading it and
                // dropping the imaginary component (mpc's
                // `LONG_DOUBLE_COMPLEX lc = c` zeroed/garbage'd cimagl(lc)).
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (is_complex(node->lhs->ty) && !is_complex(node->rhs->ty) &&
                       is_number(node->rhs->ty)) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            } else if (!is_complex(node->lhs->ty) && is_complex(node->rhs->ty) &&
                       is_number(node->lhs->ty)) {
                // Assigning a complex value to a non-complex scalar discards
                // the imaginary part (GNU extension): cast rhs down to the
                // real scalar type so codegen loads just the real component.
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->rhs;
                cast->ty = node->lhs->ty;
                cast->tok = node->rhs->tok;
                node->rhs = cast;
            }
        }
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        Type *lty = node->lhs->ty;
        Type *rty = node->rhs->ty;
        if (is_number(lty) && is_number(rty)) {
            // codeql[cpp/inconsistent-null-check]: usual_arith_type()
            // always returns a real Type* (never NULL) — every path
            // returns lhs/rhs/higher or a static ty_* global.
            Type *cmp_ty = usual_arith_type(lty, rty);
            if (is_flonum(cmp_ty)) {
                if (is_integer(lty))
                    insert_arith_cast(&node->lhs, cmp_ty);
                if (is_integer(rty))
                    insert_arith_cast(&node->rhs, cmp_ty);
            } else if (is_integer(cmp_ty) || is_complex(cmp_ty)) {
                if (cmp_ty->size > lty->size)
                    insert_arith_cast(&node->lhs, cmp_ty);
                if (cmp_ty->size > rty->size)
                    insert_arith_cast(&node->rhs, cmp_ty);
            }
        }
        node->ty = ty_int;
        return;
    }
    case ND_SIZEOF:
        node->ty = ty_int;
        return;
    case ND_POST_INC:
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC:
        node->ty = node->lhs->ty;
        return;
    case ND_COND: {
        Type *tty = node->then->ty;
        Type *ety = node->els->ty;
        // If either operand is void, the result is void
        if ((tty && tty->kind == TY_VOID) || (ety && ety->kind == TY_VOID)) {
            node->ty = ty_void;
            return;
        }
        // Null pointer constant: any integer constant expression with value 0
        // (including casts to integer types like (char)0, (bool)0, (enum e)0),
        // optionally cast to unqualified void*.
        bool then_null = is_null_pointer_constant(node->then);
        bool els_null = is_null_pointer_constant(node->els);
        if (then_null && ety && (ety->kind == TY_PTR || ety->kind == TY_ARRAY || ety->kind == TY_VLA)) {
            node->ty = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? decay_to_ptr(ety) : ety;
            return;
        }
        if (els_null && tty && (tty->kind == TY_PTR || tty->kind == TY_ARRAY || tty->kind == TY_VLA)) {
            node->ty = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? decay_to_ptr(tty) : tty;
            return;
        }
        // Both pointers: find composite type
        if (tty && ety && (tty->kind == TY_PTR || tty->kind == TY_ARRAY || tty->kind == TY_VLA) &&
            (ety->kind == TY_PTR || ety->kind == TY_ARRAY || ety->kind == TY_VLA)) {
            Type *tbase = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? tty->base : tty->base;
            Type *ebase = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? ety->base : ety->base;
            // void* combines with any pointer; qualifiers merge from both sides
            if (tbase->kind == TY_VOID || ebase->kind == TY_VOID) {
                // Pick the void* side; if both void*, pick then-side
                Type *vptr = (ebase->kind != TY_VOID) ? tty : ety;
                unsigned char combined = tbase->qual | ebase->qual;
                // C23 (PR98397): carry element qualifiers from pointer-to-array
                // types; before C23 those qualifiers are lost here
                bool c23 = opt_std_version && strcmp(opt_std_version, "202311L") >= 0;
                if (c23 && (tbase->kind == TY_ARRAY || tbase->kind == TY_VLA))
                    combined |= tbase->base->qual;
                if (c23 && (ebase->kind == TY_ARRAY || ebase->kind == TY_VLA))
                    combined |= ebase->base->qual;
                // _Atomic is not a type qualifier for this merge: the
                // composite of `_Atomic void *` and `void *` is `void *`
                combined &= (unsigned char)~QUAL_ATOMIC;
                if (vptr->base->qual != combined) {
                    Type *vbase = arena_alloc(sizeof(Type));
                    *vbase = *vptr->base;
                    vbase->qual = combined;
                    Type *result = arena_alloc(sizeof(Type));
                    *result = *vptr;
                    result->base = vbase;
                    node->ty = result;
                } else {
                    node->ty = vptr;
                }
            } else {
                // Same base kind: prefer the complete side (for incomplete arrays),
                // then combine qualifiers
                Type *chosen_ptr = (tty->kind == TY_ARRAY || tty->kind == TY_VLA) ? decay_to_ptr(tty) : tty;
                Type *other_ptr = (ety->kind == TY_ARRAY || ety->kind == TY_VLA) ? decay_to_ptr(ety) : ety;
                // If then-base is an incomplete array and els-base is complete, use els
                if (tbase->kind == TY_ARRAY && tbase->size == 0 &&
                    ebase->kind == TY_ARRAY && ebase->size > 0)
                    chosen_ptr = other_ptr;
                // For struct/union types with different scope definitions but same tag,
                // create a composite type with merged member types
                if ((tbase->kind == TY_STRUCT || tbase->kind == TY_UNION) && tbase != ebase) {
                    Type *comp_base = composite_type(tbase, ebase);
                    unsigned char combined_qual = tbase->qual | ebase->qual;
                    combined_qual &= (unsigned char)~QUAL_ATOMIC;
                    if (comp_base->qual != combined_qual) {
                        Type *rb = arena_alloc(sizeof(Type));
                        *rb = *comp_base;
                        rb->qual = combined_qual;
                        Type *rp = arena_alloc(sizeof(Type));
                        *rp = *chosen_ptr;
                        rp->base = rb;
                        node->ty = rp;
                    } else {
                        Type *rp = arena_alloc(sizeof(Type));
                        *rp = *chosen_ptr;
                        rp->base = comp_base;
                        node->ty = rp;
                    }
                    return;
                }
                unsigned char combined = chosen_ptr->base->qual | (chosen_ptr == other_ptr ? tbase->qual : ebase->qual);
                if (chosen_ptr->base->qual != combined) {
                    Type *rbase = arena_alloc(sizeof(Type));
                    *rbase = *chosen_ptr->base;
                    rbase->qual = combined;
                    Type *rptr = arena_alloc(sizeof(Type));
                    *rptr = *chosen_ptr;
                    rptr->base = rbase;
                    node->ty = rptr;
                } else {
                    node->ty = chosen_ptr;
                }
            }
            // C23: propagate [[reproducible]]/[[unsequenced]] to composite pointer type
            if (node->ty && node->ty->kind == TY_PTR && node->ty->base && node->ty->base->kind == TY_FUNC) {
                Type *fcopy = NULL;
                if (tty && tty->kind == TY_PTR && tty->base && tty->base->kind == TY_FUNC &&
                    (tty->base->is_reproducible || tty->base->is_unsequenced)) {
                    fcopy = arena_alloc(sizeof(Type));
                    *fcopy = *node->ty->base;
                    fcopy->is_reproducible |= tty->base->is_reproducible;
                    fcopy->is_unsequenced |= tty->base->is_unsequenced;
                }
                if (ety && ety->kind == TY_PTR && ety->base && ety->base->kind == TY_FUNC &&
                    (ety->base->is_reproducible || ety->base->is_unsequenced)) {
                    if (!fcopy) {
                        fcopy = arena_alloc(sizeof(Type));
                        *fcopy = *node->ty->base;
                    }
                    fcopy->is_reproducible |= ety->base->is_reproducible;
                    fcopy->is_unsequenced |= ety->base->is_unsequenced;
                }
                if (fcopy) {
                    Type *rptr = arena_alloc(sizeof(Type));
                    *rptr = *node->ty;
                    rptr->base = fcopy;
                    node->ty = rptr;
                }
            }
            return;
        }
        // Both arithmetic: usual arithmetic conversions
        if (tty && ety && is_number(tty) && is_number(ety)) {
            node->ty = usual_arith_type(tty, ety);
            if (tty != node->ty) insert_arith_cast(&node->then, node->ty);
            if (ety != node->ty) insert_arith_cast(&node->els, node->ty);
            return;
        }
        // Pointer/integer mismatch: warn and use the pointer type
        if (tty && ety && ((tty->kind == TY_PTR && is_integer(ety)) || (ety->kind == TY_PTR && is_integer(tty)))) {
            if (node->tok)
                warn_tok(node->tok, "pointer/integer mismatch in conditional expression");
        }
        node->ty = tty ? tty : ety;
        return;
    }
    case ND_COMMA:
        node->ty = node->rhs->ty;
        // Comma expressions are never lvalues; apply array/function decay.
        // rhs->ty can be NULL here: several node kinds (ND_NULL,
        // ND_ZERO_INIT, ND_LABEL, ...) are statement-like and never get a
        // type assigned above, but can still legally sit as a comma
        // operator's rightmost operand (e.g. a GNU statement-expression
        // tail, or a macro-generated `(x, (void)0)`-style idiom). Leave
        // the comma's own type NULL too in that case rather than
        // dereferencing it.
        if (node->ty) {
            if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA)
                node->ty = pointer_to(node->ty->base);
            else if (node->ty->kind == TY_FUNC)
                node->ty = pointer_to(node->ty);
        }
        return;
    case ND_NUM:
        node->ty = ty_int;
        return;
    case ND_LVAR:
        if (!node->var->ty) node->var->ty = ty_int;
        node->ty = node->var->ty;
        return;
    case ND_ADDR: {
        Node *operand = node->lhs;
        // A function designator is already represented as pointer-to-function
        // (rcc stores functions as pointer_to(TY_FUNC)). Per C, &func has the
        // same type as the decayed function designator — pointer-to-function,
        // not pointer-to-pointer-to-function. Without this, &func fails to
        // match a `T (*)(...)` type in _Generic and mis-selects the default.
        if (operand->kind == ND_LVAR && operand->var && operand->var->is_function &&
            operand->ty && operand->ty->kind == TY_PTR && operand->ty->base &&
            operand->ty->base->kind == TY_FUNC)
            node->ty = operand->ty;
        else
            node->ty = pointer_to(operand->ty);
        return;
    }
    case ND_DEREF:
        // Function-designator decay: an expression of function type
        // implicitly converts to pointer-to-function as an rvalue before
        // any operator sees it (the same "decay" arrays undergo), so
        // dereferencing an already-function-typed operand just yields the
        // function type back unchanged. Without this, repeated `*` on a
        // function name (idempotent per C11 6.5.3.2p4 - e.g.
        // "(***f)()" == "f()") fails after the first deref: *f is TY_FUNC,
        // and the plain pointer/array/VLA check below would then reject
        // the second dereference outright.
        if (node->lhs->ty->kind == TY_FUNC) {
            node->ty = node->lhs->ty;
            return;
        }
        if (node->lhs->ty->kind != TY_PTR && node->lhs->ty->kind != TY_ARRAY && node->lhs->ty->kind != TY_VLA) {
            // "pointer expected" matches tinycc's diagnostic for non-pointer
            // dereferences (tinycc tests/tests2/125_atomic_misc asserts this
            // exact text), and error_tok_simple emits no source echo/caret,
            // also matching tinycc's single-line format.
            error_tok_simple(node->tok, "pointer expected");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_CAST:
        if (!node->ty)
            node->ty = node->lhs->ty;
        return;
    case ND_BITNOT:
        node->ty = integer_promotion(node->lhs->ty);
        return;
    case ND_FUNCALL:
        if (node->funcname && builtin_return_type(node->funcname)) {
            node->ty = builtin_return_type(node->funcname);
        } else if (node->lhs && node->lhs->ty) {
            if (node->lhs->ty->kind == TY_PTR &&
                node->lhs->ty->base && node->lhs->ty->base->kind == TY_FUNC) {
                node->ty = node->lhs->ty->base->return_ty;
            } else if (node->lhs->ty->kind == TY_FUNC) {
                node->ty = node->lhs->ty->return_ty;
            } else if (node->funcname) {
                LVar *gvar = find_global_name(node->funcname);
                node->ty = (gvar && gvar->ty && gvar->ty->kind == TY_FUNC && gvar->ty->return_ty)
                    ? gvar->ty->return_ty
                    : implicit_return_type(node->funcname);
            } else {
                node->ty = ty_int;
            }
        } else if (node->funcname) {
            LVar *gvar = find_global_name(node->funcname);
            node->ty = (gvar && gvar->ty && gvar->ty->kind == TY_FUNC && gvar->ty->return_ty)
                ? gvar->ty->return_ty
                : implicit_return_type(node->funcname);
        } else {
            node->ty = ty_int;
        }
        // Insert implicit casts for arguments when prototype is available
        Type *param_types = NULL;
        if (node->lhs && node->lhs->ty && node->lhs->ty->kind == TY_PTR &&
            node->lhs->ty->base && node->lhs->ty->base->kind == TY_FUNC)
            param_types = node->lhs->ty->base->param_types;
        else if (node->lhs && node->lhs->ty && node->lhs->ty->kind == TY_FUNC)
            param_types = node->lhs->ty->param_types;
        for (Node *n = node->args; n; n = n->next)
            check_type(n);
        // Insert implicit casts for arguments when prototype is available
        for (Node **argp = &node->args; *argp && param_types;
             argp = &(*argp)->next, param_types = param_types->param_next) {
            // Struct/union arguments are passed by raw value copy
            // (codegen classifies/copies them from the argument's own,
            // already-resolved type) -- never coerced via an
            // arithmetic-conversion-style cast. same_type() falls back to
            // a size compare for aggregates, which wrongly says "not the
            // same type" when a struct tag is forward-declared (typedef'd
            // and used in an earlier prototype, e.g. zstd's
            // `ZSTD_CCtx_params`) and only completed later in the same TU:
            // the prototype's parameter Type object froze the incomplete
            // size (0) at declaration time, while the argument's own
            // variable observes the real completed struct body/size
            // (rcc's struct tags are identity objects completed in place
            // -- see copy_type()'s own comment -- but two SEPARATE
            // Type objects for the same tag can still exist when one was
            // captured before completion and never re-synced). Wrapping
            // the argument in a cast to that stale, zero-size type here
            // silently discarded the real size at every later ABI
            // decision (register/stack classification, byte-copy loop
            // count), corrupting the call. cast_funcall_args() (parser.c)
            // already deliberately excludes struct/union from its own,
            // narrower is_integer/is_flonum cast check for the same
            // reason; mirror that here.
            if (((*argp)->ty && ((*argp)->ty->kind == TY_STRUCT || (*argp)->ty->kind == TY_UNION)) ||
                (param_types->kind == TY_STRUCT || param_types->kind == TY_UNION))
                continue;
            if (same_type((*argp)->ty, param_types))
                continue;
            // GCC transparent_union (__attribute__((__transparent_union__))):
            // the argument is passed using the calling convention of
            // whichever member type it matches, not boxed into the union —
            // there is nothing to actually convert. Casting it to TY_UNION
            // here would make codegen treat a plain pointer argument as an
            // aggregate needing its address, which it isn't (see
            // include/crypto/aes.h's aes_encrypt_arg, passed a bare
            // `struct aes_enckey *`/`struct aes_key *`).
            if (param_types->kind == TY_UNION && param_types->is_transparent_union &&
                (*argp)->ty && (*argp)->ty->kind == TY_PTR) {
                bool matches_member = false;
                for (Member *m = param_types->members; m; m = m->next)
                    if (m->ty->kind == TY_PTR) {
                        matches_member = true;
                        break;
                    }
                if (matches_member) continue;
            }
            insert_arith_cast(argp, param_types);
        }
        return;
    case ND_STR:
        node->ty = pointer_to(ty_char);
        return;
    case ND_MEMBER: {
        // A member accessed through a const- (or volatile-) qualified
        // struct/union expression is itself qualified, even when the
        // member's OWN declared type carries no qualifier at all (C11
        // 6.5.2.3p3: "the result has ... the type qualifiers of the
        // specified member") -- e.g. `const struct S *sp` with a plain
        // `char chars[16]` member: `&sp->chars[0]` must be `const char *`,
        // not `char *`. Previously this was entirely unpropagated: an
        // access through a const struct pointer silently produced the
        // member's bare, unqualified type, so a `const`-vs-unqualified
        // __builtin_types_compatible_p() check on the result (postgres's
        // own `unconstify()` macro asserting it) always disagreed with
        // real GCC. `node->lhs` is the STRUCT/UNION VALUE being accessed
        // (already dereferenced for `->`, see parser.c's `->` handling),
        // so its own `->ty->qual` is exactly the qualifier set to inherit.
        unsigned char inherit = node->lhs->ty ? (node->lhs->ty->qual & (QUAL_CONST | QUAL_VOLATILE)) : 0;
        Type *base_ty;
        if (node->member->bit_width > 0) {
            int bw = node->member->bit_width;
            bool bf_unsigned = node->member->ty->is_unsigned;
            // 64-bit declared type (long long / unsigned long long): keep as-is
            // so that sizeof(s->field + 0) == 8 and %016llx format is selected
            if (node->member->ty->size >= 8)
                base_ty = node->member->ty;
            else if (bw < 32 || (bw == 32 && !bf_unsigned))
                base_ty = ty_int;
            else if (bw == 32)
                base_ty = ty_uint;
            else
                base_ty = node->member->ty;
        } else {
            base_ty = node->member->ty;
        }
        unsigned char missing = inherit & ~base_ty->qual;
        node->ty = missing ? qualify_type_copy(base_ty, missing) : base_ty;
        return;
    }
    case ND_STMT_EXPR: {
        if (node->stmt_expr_result) {
            add_type_internal(node->stmt_expr_result);
            node->ty = node->stmt_expr_result->ty;
        } else {
            node->ty = ty_int;
        }
        return;
    }
    case ND_DO:
    case ND_SWITCH:
    case ND_CASE:
    case ND_BREAK:
    case ND_CONTINUE:
    case ND_GOTO:
    case ND_GOTO_IND:
    case ND_LABEL:
    case ND_NULL:
    case ND_ZERO_INIT:
        return;
    case ND_LABEL_VAL:
        // type is already set to void* in parser
        return;
    default:
        return;
    }
}

void check_type(Node *node) {
    add_type_internal(node);
}

Type *vla_of(Type *base, Node *len, int64_t arr_len) {
    Type *ty = arena_alloc(sizeof(Type));
    ty->kind = TY_VLA;
    ty->size = 16; // 8 for array base ptr + 8 for saved RSP
    ty->align = 8;
    ty->base = base;
    if (len) {
        check_type(len);
        ty->vla_len_expr = len;
    } else {
        ty->array_len = arr_len;
    }
    return ty;
}
