/* rcc decimal runtime wrapper layer.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Provides the __bid_*3/__bid_*2 entry points that rcc's decimal codegen
 * emits (the same symbol names GCC and kefir use for _Decimal32/64/128
 * arithmetic), but with a plain bit-pattern ABI instead of GCC's
 * _Decimal32/64/128 C types: every value is passed as its raw BID
 * encoding (UINT32 / UINT64 / 16-byte UINT128). This is required so
 * the runtime compiles on targets whose compiler has no _Decimal types
 * (aarch64 gcc rejects them), and it is sufficient because the callers
 * (rcc-generated code) and the callees (this file) agree on the ABI.
 *
 * The heavy lifting is done by the pure-C libbid core files vendored
 * alongside this file (bid64_add.c, bid128_div.c, bid_binarydecimal.c,
 * ...), which implement IEEE 754-2008 decimal arithmetic in BID
 * (binary-integer-decimal) encoding. libbid is part of libdfp
 * (LGPL-2.1-or-later, same as rcc) and of GCC's libgcc (GPLv3 + GCC
 * Runtime Library Exception); the relevant license texts are in the
 * COPYING* files in this directory.
 *
 * ABI notes (matching the kefir/gcc lowering):
 *   __bid_{add,sub,mul,div}{sd,dd,td}3 : 2 operands -> 1 result
 *   __bid_{eq,ne,lt,le,gt,ge,unord}{sd,dd,td}2 : 2 operands -> int
 *   __bid_{extend,trunc}*2 : 1 operand -> 1 result (size-changing)
 *   __bid_fix{uns}{sd,dd,td}{si,di,ti} : decimal -> int (truncate)
 *   __bid_float{,uns}{si,di,ti}{sd,dd,td} : int -> decimal
 *   __bid_{sd,dd,td}_from_string / to_string : literals and printf
 *
 * All values are passed/returned by value except decimal128 (UINT128),
 * which is a 16-byte struct passed per the platform ABI for two-word
 * structs (like rcc's own __int128 values).
 */

#include "bid_conf.h"
#include "bid_functions.h"
#include "bid_internal.h"

/* ---------- decimal32 (7 digits, 32-bit BID) ---------- */

UINT32 __bid_addsd3(UINT32 x, UINT32 y) {
    UINT64 x64 = __bid32_to_bid64(x), y64 = __bid32_to_bid64(y);
    return (UINT32)__bid64_to_bid32(__bid64_add(x64, y64));
}
UINT32 __bid_subsd3(UINT32 x, UINT32 y) {
    UINT64 x64 = __bid32_to_bid64(x), y64 = __bid32_to_bid64(y);
    return (UINT32)__bid64_to_bid32(__bid64_sub(x64, y64));
}
UINT32 __bid_mulsd3(UINT32 x, UINT32 y) {
    UINT64 x64 = __bid32_to_bid64(x), y64 = __bid32_to_bid64(y);
    return (UINT32)__bid64_to_bid32(__bid64_mul(x64, y64));
}
UINT32 __bid_divsd3(UINT32 x, UINT32 y) {
    UINT64 x64 = __bid32_to_bid64(x), y64 = __bid32_to_bid64(y);
    return (UINT32)__bid64_to_bid32(__bid64_div(x64, y64));
}
int __bid_eqsd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_equal(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_nesd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_not_equal(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_ltsd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_less(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_lesd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_less_equal(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_gtsd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_greater(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_gesd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_greater_equal(__bid32_to_bid64(x), __bid32_to_bid64(y));
}
int __bid_unordsd2(UINT32 x, UINT32 y) {
    return __bid64_quiet_unordered(__bid32_to_bid64(x), __bid32_to_bid64(y));
}

/* ---------- decimal64 (16 digits, 64-bit BID) ---------- */

UINT64 __bid_adddd3(UINT64 x, UINT64 y) { return __bid64_add(x, y); }
UINT64 __bid_subdd3(UINT64 x, UINT64 y) { return __bid64_sub(x, y); }
UINT64 __bid_muldd3(UINT64 x, UINT64 y) { return __bid64_mul(x, y); }
UINT64 __bid_divdd3(UINT64 x, UINT64 y) { return __bid64_div(x, y); }
int __bid_eqdd2(UINT64 x, UINT64 y) { return __bid64_quiet_equal(x, y); }
int __bid_nedd2(UINT64 x, UINT64 y) { return __bid64_quiet_not_equal(x, y); }
int __bid_ltdd2(UINT64 x, UINT64 y) { return __bid64_quiet_less(x, y); }
int __bid_ledd2(UINT64 x, UINT64 y) { return __bid64_quiet_less_equal(x, y); }
int __bid_gtdd2(UINT64 x, UINT64 y) { return __bid64_quiet_greater(x, y); }
int __bid_gedd2(UINT64 x, UINT64 y) { return __bid64_quiet_greater_equal(x, y); }
int __bid_unorddd2(UINT64 x, UINT64 y) { return __bid64_quiet_unordered(x, y); }

/* ---------- decimal128 (34 digits, 128-bit BID) ---------- */

UINT128 __bid_addtd3(UINT128 x, UINT128 y) { return __bid128_add(x, y); }
UINT128 __bid_subtd3(UINT128 x, UINT128 y) { return __bid128_sub(x, y); }
UINT128 __bid_multd3(UINT128 x, UINT128 y) { return __bid128_mul(x, y); }
UINT128 __bid_divtd3(UINT128 x, UINT128 y) { return __bid128_div(x, y); }
int __bid_eqtd2(UINT128 x, UINT128 y) { return __bid128_quiet_equal(x, y); }
int __bid_netd2(UINT128 x, UINT128 y) { return __bid128_quiet_not_equal(x, y); }
int __bid_lttd2(UINT128 x, UINT128 y) { return __bid128_quiet_less(x, y); }
int __bid_letd2(UINT128 x, UINT128 y) { return __bid128_quiet_less_equal(x, y); }
int __bid_gttd2(UINT128 x, UINT128 y) { return __bid128_quiet_greater(x, y); }
int __bid_getd2(UINT128 x, UINT128 y) { return __bid128_quiet_greater_equal(x, y); }
int __bid_unordtd2(UINT128 x, UINT128 y) { return __bid128_quiet_unordered(x, y); }

/* ---------- size-changing conversions ---------- */

UINT64 __bid_extendsddd2(UINT32 x) { return __bid32_to_bid64(x); }
UINT128 __bid_extendsdtd2(UINT32 x) { return __bid32_to_bid128(x); }
UINT128 __bid_extendddtd2(UINT64 x) { return __bid64_to_bid128(x); }
UINT32 __bid_truncddsd2(UINT64 x) { return (UINT32)__bid64_to_bid32(x); }
UINT32 __bid_trunctdsd2(UINT128 x) { return (UINT32)__bid128_to_bid32(x); }
UINT64 __bid_trunctddd2(UINT128 x) { return __bid128_to_bid64(x); }

/* ---------- decimal <-> int conversions (truncate toward zero) ---------- */

UINT32 __bid_floatsisd(SINT32 x) { return (UINT32)__bid64_to_bid32(__bid64_from_int32(x)); }
UINT32 __bid_floatdisd(SINT64 x) { return (UINT32)__bid64_to_bid32(__bid64_from_int64(x)); }
UINT32 __bid_floatunssisd(UINT32 x) { return (UINT32)__bid64_to_bid32(__bid64_from_uint32(x)); }
UINT32 __bid_floatunsdisd(UINT64 x) { return (UINT32)__bid64_to_bid32(__bid64_from_uint64(x)); }
UINT64 __bid_floatsidd(SINT32 x) { return __bid64_from_int32(x); }
UINT64 __bid_floatdidd(SINT64 x) { return __bid64_from_int64(x); }
UINT64 __bid_floatunssidd(UINT32 x) { return __bid64_from_uint32(x); }
UINT64 __bid_floatunsdidd(UINT64 x) { return __bid64_from_uint64(x); }
UINT128 __bid_floatsitd(SINT32 x) { return __bid64_to_bid128(__bid64_from_int32(x)); }
UINT128 __bid_floatditd(SINT64 x) { return __bid64_to_bid128(__bid64_from_int64(x)); }
UINT128 __bid_floatunstitd(UINT32 x) { return __bid64_to_bid128(__bid64_from_uint32(x)); }
UINT128 __bid_floatunstitd2(UINT64 x) { return __bid64_to_bid128(__bid64_from_uint64(x)); }

SINT32 __bid_fixsdsi(UINT32 x) { return (SINT32)__bid64_to_int32_int(__bid32_to_bid64(x)); }
SINT64 __bid_fixsddi(UINT32 x) { return (SINT64)__bid64_to_int64_int(__bid32_to_bid64(x)); }
SINT32 __bid_fixddsi(UINT64 x) { return (SINT32)__bid64_to_int32_int(x); }
SINT64 __bid_fixdddi(UINT64 x) { return (SINT64)__bid64_to_int64_int(x); }
SINT32 __bid_fixtdsi(UINT128 x) { return (SINT32)__bid128_to_int32_int(x); }
SINT64 __bid_fixtddi(UINT128 x) { return (SINT64)__bid128_to_int64_int(x); }
UINT32 __bid_fixunssdsi(UINT32 x) { return (UINT32)__bid64_to_uint32_int(__bid32_to_bid64(x)); }
UINT64 __bid_fixunssddi(UINT32 x) { return (UINT64)__bid64_to_uint64_int(__bid32_to_bid64(x)); }
UINT32 __bid_fixunsddsi(UINT64 x) { return (UINT32)__bid64_to_uint32_int(x); }
UINT64 __bid_fixunsdddi(UINT64 x) { return (UINT64)__bid64_to_uint64_int(x); }
UINT32 __bid_fixunstdsi(UINT128 x) { return (UINT32)__bid128_to_uint32_int(x); }
UINT64 __bid_fixunstddi(UINT128 x) { return (UINT64)__bid128_to_uint64_int(x); }

/* ---------- decimal <-> binary FP conversions ---------- */

UINT32 __bid_extendsfsd(float f) { return (UINT32)__bid64_to_bid32(__binary32_to_bid64(f)); }
UINT32 __bid_truncdfsd(double d) { return (UINT32)__bid64_to_bid32(__binary64_to_bid64(d)); }
UINT64 __bid_extendsfdd(float f) { return __binary32_to_bid64(f); }
UINT64 __bid_extenddfdd(double d) { return __binary64_to_bid64(d); }
UINT128 __bid_extendsftd(float f) { return __binary32_to_bid128(f); }
UINT128 __bid_extenddftd(double d) { return __binary64_to_bid128(d); }
float __bid_truncsdsf(UINT32 x) { return __bid32_to_binary32(x); }
double __bid_extendsddf(UINT32 x) { return __bid32_to_binary64(x); }
float __bid_truncddsf(UINT64 x) { return __bid64_to_binary32(x); }
double __bid_truncdddf(UINT64 x) { return __bid64_to_binary64(x); }
float __bid_trunctdsf(UINT128 x) { return __bid128_to_binary32(x); }
double __bid_trunctddf(UINT128 x) { return __bid128_to_binary64(x); }

/* ---------- string conversions (compile-time literals, printf) ---------- */

// __bid64_from_string / __bid128_from_string come from bid64_string.c /
// bid128_string.c (bid_conf.h renames bid*_from_string to __bid*_from_string).
// Only decimal32 needs a shim (libbid has no bid32 string parser).
void __bid32_from_string(UINT32 *pres, const char *ps) {
    UINT64 r = bid64_from_string((char *)ps);
    *pres = (UINT32)__bid64_to_bid32(r);
}
