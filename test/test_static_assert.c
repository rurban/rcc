// https://en.cppreference.com/c/language/_Static_assert
// #include <assert.h> // no longer needed since C23

int main(void)
{
    // Test if math works, C23:
    static_assert((2 + 2) % 3 == 1, "Whoa dude, you knew!");
    // Pre-C23 alternative:
    _Static_assert(2 + 2 * 2 == 6, "Lucky guess!?");

    // This will produce an error at compile time.
    // static_assert(sizeof(int) < sizeof(char), "Unmet condition!");

    constexpr int _42 = 2 * 3 * 2 * 3 + 2 * 3;
    static_assert(_42 == 42); // the message string can be omitted.

    // const int _13 = 13;
    // Compile time error - not an integer constant expression:
    // static_assert(_13 == 13);

    // Bit-counting builtins must constant-fold with a compile-time-constant
    // argument, same as GCC — the kernel's bits_per()/ilog2() (linux/log2.h)
    // guard their non-constant fallback behind __builtin_constant_p(n) and
    // still need clz/ctz/popcount/ffs itself to fold once that branch is
    // selected, or the surrounding static_assert is rejected as non-constant.
    static_assert(__builtin_clz(1u) == 31, "clz(1)");
    static_assert(__builtin_clz(0x80000000u) == 0, "clz(0x80000000)");
    static_assert(__builtin_clzl(1ul) == (sizeof(long) * 8 - 1), "clzl(1)");
    static_assert(__builtin_clzll(1ull) == 63, "clzll(1)");

    static_assert(__builtin_ctz(0x80000000u) == 31, "ctz(0x80000000)");
    static_assert(__builtin_ctz(1u) == 0, "ctz(1)");
    static_assert(__builtin_ctzl(2ul) == 1, "ctzl(2)");
    static_assert(__builtin_ctzll(1ull << 40) == 40, "ctzll(1<<40)");

    static_assert(__builtin_popcount(0xFFu) == 8, "popcount(0xFF)");
    static_assert(__builtin_popcount(0u) == 0, "popcount(0)");
    static_assert(__builtin_popcountl(0xFul) == 4, "popcountl(0xF)");
    static_assert(__builtin_popcountll(~0ull) == 64, "popcountll(~0)");

    static_assert(__builtin_ffs(0) == 0, "ffs(0)");
    static_assert(__builtin_ffs(1) == 1, "ffs(1)");
    static_assert(__builtin_ffs(8) == 4, "ffs(8)");
    static_assert(__builtin_ffsl(16l) == 5, "ffsl(16)");
    static_assert(__builtin_ffsll(1ll << 40) == 41, "ffsll(1<<40)");

    // __builtin_bswap16/32/64 must constant-fold too — the kernel's
    // __swab16/32/64 (linux/swab.h) expand to these and are used as
    // case labels / in static_assert throughout networking headers.
    static_assert(__builtin_bswap16(0x1234) == 0x3412, "bswap16");
    static_assert(__builtin_bswap32(0x12345678) == 0x78563412u, "bswap32");
    static_assert(__builtin_bswap64(0x123456789abcdef0ULL) == 0xf0debc9a78563412ULL, "bswap64");
}
