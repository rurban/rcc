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

    // Combined with existing features:
    constexpr int million = 1'000'000;
    static_assert(million == 1000000);

    printf("PASS\n");
    return 0;
}
