// C23 enum type-modeling: (1) `enum tag : type` fixed underlying type must
// give the enum itself, and every enumerator, that exact underlying type
// (kind/size/signedness) — not a generic int — so
// __builtin_types_compatible_p(enum tag, underlying_type) and
// typeof(enumerator) both see it; (2) with no fixed underlying type, each
// enumerator outside int's range gets the smallest of
// int/unsigned/long/unsigned long/long long/unsigned long long that can
// represent it (GCC/clang extension, C23 6.7.3.4), and the enum's own type
// is the narrowest of that ladder that can hold every enumerator, walking
// through long/unsigned long (not jumping straight from int/unsigned to
// long long/unsigned long long); (3) __builtin_types_compatible_p must
// treat two separately declared enums (named or anonymous) as incompatible
// even when they share identical size/signedness/enumerator values — enum
// identity is nominal, not structural, just like struct/union.
// See test/third_party/test_cproc/cproc/test/enum-fixed.c and
// enum-large-value.c (upstream cproc test corpus).

// --- (1) fixed underlying type ---

enum Small : short;
static_assert(__builtin_types_compatible_p(enum Small, short));

enum Small : short { SMALL_A = 1, SMALL_B = 2 };
static_assert(__builtin_types_compatible_p(enum Small, short));
static_assert(__builtin_types_compatible_p(typeof(SMALL_A), short));
static_assert(sizeof(enum Small) == sizeof(short));

enum Wide : unsigned long long {
    WIDE_A = 1,
    // typeof() used *inside* the same enum body, before it "closes", must
    // already report the fixed underlying type for an enumerator declared
    // earlier in the same list.
    WIDE_A_IS_ULL = __builtin_types_compatible_p(typeof(WIDE_A), unsigned long long),
};
static_assert(WIDE_A_IS_ULL == 1);
static_assert(__builtin_types_compatible_p(typeof(WIDE_A), unsigned long long));
static_assert(__builtin_types_compatible_p(enum Wide, unsigned long long));

// --- (2) value-range-based promotion (no fixed underlying type) ---

enum PromoteUnsigned { PU_A = 0x80000000 }; // doesn't fit int, fits unsigned
static_assert(__builtin_types_compatible_p(typeof(PU_A), unsigned));
static_assert(__builtin_types_compatible_p(enum PromoteUnsigned, unsigned));

// `long`'s width is platform-dependent: 8 bytes on LP64 (Linux/macOS),
// 4 bytes on LLP64 (Windows/mingw, where it has the same range as `int`).
// A value needing more than 32 signed bits therefore promotes to `long`
// only where `long` is actually wide enough to hold it; on LLP64 it must
// skip straight to `long long`, exactly as GCC's own enum finalization
// does (see test/torture/c23-enum-1.c's analogous __LONG_MAX__ guards).
#if __LONG_MAX__ > __INT_MAX__
typedef long wide_signed_t;
typedef unsigned long wide_unsigned_t;
#else
typedef long long wide_signed_t;
typedef unsigned long long wide_unsigned_t;
#endif

enum PromoteLong {
    PL_A = 0x80000000, // fits unsigned
    PL_B = -1,          // forces signed representation for the whole enum
};
static_assert(__builtin_types_compatible_p(enum PromoteLong, wide_signed_t));

enum PromoteUnsignedLong {
    PUL_A = 0x100000000,       // needs > 32 bits, fits unsigned long
    PUL_B = 0xFFFFFFFFFFFFFFFFULL, // needs exactly 64 bits unsigned
    // During processing (before the enum closes) each enumerator keeps its
    // own individual type — PUL_B's is its literal's natural type.
    PUL_B_DURING = __builtin_types_compatible_p(typeof(PUL_B), unsigned long long),
};
static_assert(PUL_B_DURING == 1);
// Once the enum closes, every enumerator is retyped to the enum's own
// overall type — the narrowest of int/long/long long (or their unsigned
// counterparts) that holds every value, walking through long before long
// long, so a 64-bit-precision unsigned enum lands on "unsigned long" where
// `long` is 64 bits wide, or "unsigned long long" where it's only 32.
static_assert(__builtin_types_compatible_p(typeof(PUL_B), wide_unsigned_t));
static_assert(__builtin_types_compatible_p(enum PromoteUnsignedLong, wide_unsigned_t));

// --- (3) enum identity: same shape, different declaration => incompatible ---

enum Tag1 { TAG1_A = 1, TAG1_B = 2 };
enum Tag2 { TAG2_A = 1, TAG2_B = 2 }; // structurally identical to Tag1
static_assert(!__builtin_types_compatible_p(enum Tag1, enum Tag2));
static_assert(!__builtin_types_compatible_p(enum Tag1, enum { X1 = 1, Y1 = 2 }));
static_assert(!__builtin_types_compatible_p(enum { X2 = 1 }, enum { Y2 = 1 }));
// ...but referencing the *same* tag twice is still compatible.
static_assert(__builtin_types_compatible_p(enum Tag1, enum Tag1));
// enum-vs-plain-integer-type compatibility (matching representation) is
// unaffected by the identity rule — only enum-vs-enum needs it.
static_assert(__builtin_types_compatible_p(enum PromoteUnsigned, unsigned));

int main(void) {
    enum Small s = SMALL_B;
    if (s != 2)
        return 1;
    if ((short)s != 2)
        return 2;

    enum Wide w = WIDE_A;
    if (w != 1)
        return 3;

    if (PU_A != (int)0x80000000)
        return 4;
    if ((unsigned)PU_A != 0x80000000u)
        return 5;

    if (PL_B != -1)
        return 6;

    if (PUL_A != 0x100000000LL)
        return 7;
    if (PUL_B != 0xFFFFFFFFFFFFFFFFULL)
        return 8;

    return 0;
}
