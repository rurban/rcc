// from https://en.cppreference.com/c/numeric/bit_manip
#include <limits.h>
#include <stdbit.h>
#include <stdint.h>
#include <stdio.h>

#define bits_num(value) (sizeof(value) * CHAR_BIT)

#define bin_impl(T, suffix) \
const char* bin_##suffix(T x) \
{ \
    static char buf[bits_num(x) * CHAR_BIT + 1]; \
    for (T i = 0, mask = ((T)1 << (bits_num(x) - 1)); mask; mask >>= 1) \
        buf[i++] = x & mask ? '1' : '0'; \
    buf[bits_num(x)] = '\0'; \
    return buf; \
}

bin_impl(uint8_t, u8)
bin_impl(uint16_t, u16)
bin_impl(uint32_t, u32)
bin_impl(uint64_t, u64)

#define bin(x) _Generic((x), \
    uint8_t: bin_u8, uint16_t: bin_u16, uint32_t: bin_u32, default: bin_u64)(x)

int main()
{
    puts("uint8_t:");
    for (uint8_t x = 0b11000000; ; x >>= 1)
    {
        printf("x = [%s], leading zeros: %d\n", bin(x), stdc_leading_zeros(x));
        if (!x)
            break;
    }

    puts("uint16_t:");
    for (uint16_t x = 0b11000000; ; x >>= 1)
    {
        printf("x = [%s], leading zeros: %d\n", bin(x), stdc_leading_zeros(x));
        if (!x)
            break;
    }
}
