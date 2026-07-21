// GNU extension: enum { ... } __attribute__((packed)) narrows the
// underlying type to the smallest integer type that fits the value range,
// instead of the C-standard "at least int". The Linux kernel relies on
// this (e.g. arch/x86/include/asm/cpuid/types.h) together with
// static_assert(sizeof(enum ...) == 1) to keep such enums tightly packed.

enum unpacked_small { UNPACKED_A = 1, UNPACKED_B, UNPACKED_C };
static_assert(sizeof(enum unpacked_small) == sizeof(int), "non-packed enum must default to int-sized storage");

enum packed_u8 {
    PACKED_U8_A = 1,
    PACKED_U8_B,
    PACKED_U8_C = 255,
} __attribute__((packed));
static_assert(sizeof(enum packed_u8) == 1, "packed enum with 0..255 range must be 1 byte");

enum packed_u16 {
    PACKED_U16_A = 0,
    PACKED_U16_B = 65535,
} __attribute__((packed));
static_assert(sizeof(enum packed_u16) == 2, "packed enum with 0..65535 range must be 2 bytes");

enum packed_i8 {
    PACKED_I8_NEG = -100,
    PACKED_I8_POS = 100,
} __attribute__((packed));
static_assert(sizeof(enum packed_i8) == 1, "packed enum with -128..127 range must be 1 byte");

enum packed_i16 {
    PACKED_I16_NEG = -30000,
    PACKED_I16_POS = 30000,
} __attribute__((packed));
static_assert(sizeof(enum packed_i16) == 2, "packed enum with -32768..32767 range must be 2 bytes");

// A range that doesn't fit any narrow type still falls back to unsigned/int.
enum packed_still_uint {
    PACKED_UI_A = 0,
    PACKED_UI_B = 70000,
} __attribute__((packed));
static_assert(sizeof(enum packed_still_uint) == 4, "packed enum with range > 65535 must fall back to 4 bytes");

int main(void)
{
    enum packed_u8 a = PACKED_U8_C;
    if (a != 255)
        return 1;

    enum packed_i16 b = PACKED_I16_NEG;
    if (b != -30000)
        return 1;

    // A struct embedding a packed enum should see the narrowed size, not
    // padded up — this is the shape that trips up naive enum handling.
    struct wrapper {
        enum packed_u8 kind;
        char pad[3];
    };
    static_assert(sizeof(struct wrapper) == 4, "packed enum member must not silently widen its struct");

    return 0;
}
