// C23 digit separators, #warning, #error
// https://en.cppreference.com/c/language/integer_constant

#include <stdio.h>

#warning "this is a test warning (expected)"

int main(void)
{
    // C23 digit separators in decimal
    if (1'000'000 != 1000000) return 1;
    // C23 digit separators in hex
    if (0xFF'FF != 65535) return 2;
    // C23 digit separators in binary
    if (0b10'10'10 != 42) return 3;
    // C23 digit separators in float
    if (1'234.5'6 != 1234.56) return 4;

    // C23 0b binary literals
    if (0b101010 != 42) return 5;
    if (0b0 != 0) return 6;
    if (0b1 != 1) return 7;
    if (0b11111111 != 255) return 8;
    // binary with digit separators
    if (0b1111'0000 != 240) return 9;
    if (0b1'0'1'0'1'0 != 42) return 10;
    // binary + constexpr
    constexpr int mask = 0b1111'0000;
    static_assert(mask == 240);
    // Combined with existing features:
    constexpr int million = 1'000'000;
    static_assert(million == 1000000);

    printf("PASS\n");
    return 0;
}
