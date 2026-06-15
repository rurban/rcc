#include <limits.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdio.h>

const char* text(bool overflow) { return overflow ? "Overflow" : "OK"; }

int main()
{
    {
        uint8_t x;
        uint16_t y;
        uint16_t result_uint16;
        uint32_t result_uint32;
        bool overflow;

        x = 14;
        y = 28;
        overflow = ckd_add(&result_uint16, x, y);
        printf("%u + %u => %u (%s)\n", x, y, result_uint16, text(overflow));

        x = 14;
        y = UINT16_MAX;
        overflow = ckd_add(&result_uint16, x, y);
        printf("%u + %u => %u (%s)\n", x, y, result_uint16, text(overflow));

        x = 14;
        y = UINT16_MAX;
        overflow = ckd_add(&result_uint32, x, y);
        printf("%u + %u => %u (%s)\n", x, y, result_uint32, text(overflow));
    }
    {
        int16_t result1;
        constexpr int8_t x1 = -2;
        constexpr int16_t y1 = 1;
        const bool underflow1 = ckd_sub(&result1, x1, y1);
        printf("%i - %i == %i (%s)\n", x1, y1, result1, underflow1 ? "Underflow" : "OK");

        int16_t result2;
        constexpr int8_t x2 = -2;
        constexpr int16_t y2 = INT16_MAX;
        const bool underflow2 = ckd_sub(&result2, x2, y2);
        printf("%i - %i == %i (%s)\n", x2, y2, result2, underflow2 ? "Underflow" : "OK");

        int32_t result3;
        constexpr int8_t x3 = -2;
        constexpr int16_t y3 = INT16_MAX;
        const bool underflow3 = ckd_sub(&result3, x3, y3);
        printf("%i - %i == %i (%s)\n", x3, y3, result3, underflow3 ? "Underflow" : "OK");
    }

    {
        uint8_t x;
        uint16_t y;
        uint16_t result_uint16;
        uint32_t result_uint32;
        bool overflow;

        x = 2;
        y = 21;
        overflow = ckd_mul(&result_uint16, x, y);
        printf("%u * %u => %u (%s)\n", x, y, result_uint16, text(overflow));

        x = 2;
        y = UINT16_MAX;
        overflow = ckd_mul(&result_uint16, x, y);
        printf("%u * %u => %u (%s)\n", x, y, result_uint16, text(overflow));

        x = 2;
        y = UINT16_MAX;
        overflow = ckd_mul(&result_uint32, x, y);
        printf("%u * %u => %u (%s)\n", x, y, result_uint32, text(overflow));
    }
}

