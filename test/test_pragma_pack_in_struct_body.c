/* Regression: #pragma pack inside a struct/union body must be tolerated
 * (treated as a no-op), not rejected by the member-declaration parser.
 * Python's _ctypes_test_generated.c.h uses this idiom for MS-bitfield
 * examples; GCC ignores such in-body pragmas because the enclosing
 * type's layout is already fixed. */
#include <stdint.h>
#define GCC_ATTR(x) __attribute__((x))

struct GCC_ATTR(ms_struct) Example_gh_84039_good {
#pragma pack(push, 1)
    struct GCC_ATTR(ms_struct) {
        uint8_t a0 :1;
        uint8_t a1 :1;
        uint8_t a2 :1;
        uint8_t a3 :1;
        uint8_t a4 :1;
        uint8_t a5 :1;
        uint8_t a6 :1;
        uint8_t a7 :1;
    } a;
#pragma pack(pop)
    uint16_t b0 :4;
    uint16_t b1 :12;
};
#pragma pack(pop)

int main(void) {
    struct Example_gh_84039_good value;
    if (sizeof(value) != 4)
        return 1;
    /* _Alignof for the struct should be at least 1. */
    if (_Alignof(struct Example_gh_84039_good) < 1)
        return 2;
    (void)value.a.a0;
    (void)value.b1;
    return 0;
}
