#ifndef _STDBIT_H
#define _STDBIT_H

#include <limits.h>

#define __STDBIT_WIDTH(T)  ((int)(sizeof(T) * CHAR_BIT))

/* C23 type-generic and suffixed bit operations, implemented with
   the GCC-style builtins rcc already provides.  The suffixed forms
   delegate to static inline functions so that the type-generic
   _Generic() associations can select a callable expression. */

enum {
    __rcc_ui_width = __STDBIT_WIDTH(unsigned int),
    __rcc_uc_width = __STDBIT_WIDTH(unsigned char),
    __rcc_us_width = __STDBIT_WIDTH(unsigned short),
    __rcc_ul_width = __STDBIT_WIDTH(unsigned long),
    __rcc_ull_width = __STDBIT_WIDTH(unsigned long long)
};

static inline int __rcc_stdc_leading_zeros_uc(unsigned char x) {
    return x ? __builtin_clz((unsigned int)x) - (__rcc_ui_width - __rcc_uc_width)
             : __rcc_uc_width;
}
static inline int __rcc_stdc_leading_zeros_us(unsigned short x) {
    return x ? __builtin_clz((unsigned int)x) - (__rcc_ui_width - __rcc_us_width)
             : __rcc_us_width;
}
static inline int __rcc_stdc_leading_zeros_ui(unsigned int x) {
    return x ? __builtin_clz(x) : __rcc_ui_width;
}
static inline int __rcc_stdc_leading_zeros_ul(unsigned long x) {
    return x ? __builtin_clzl(x) : __rcc_ul_width;
}
static inline int __rcc_stdc_leading_zeros_ull(unsigned long long x) {
    return x ? __builtin_clzll(x) : __rcc_ull_width;
}

static inline int __rcc_stdc_trailing_zeros_uc(unsigned char x) {
    return x ? __builtin_ctz((unsigned int)x) : __rcc_uc_width;
}
static inline int __rcc_stdc_trailing_zeros_us(unsigned short x) {
    return x ? __builtin_ctz((unsigned int)x) : __rcc_us_width;
}
static inline int __rcc_stdc_trailing_zeros_ui(unsigned int x) {
    return x ? __builtin_ctz(x) : __rcc_ui_width;
}
static inline int __rcc_stdc_trailing_zeros_ul(unsigned long x) {
    return x ? __builtin_ctzl(x) : __rcc_ul_width;
}
static inline int __rcc_stdc_trailing_zeros_ull(unsigned long long x) {
    return x ? __builtin_ctzll(x) : __rcc_ull_width;
}

static inline int __rcc_stdc_count_ones_uc(unsigned char x) {
    return __builtin_popcount((unsigned int)x);
}
static inline int __rcc_stdc_count_ones_us(unsigned short x) {
    return __builtin_popcount((unsigned int)x);
}
static inline int __rcc_stdc_count_ones_ui(unsigned int x) {
    return __builtin_popcount(x);
}
static inline int __rcc_stdc_count_ones_ul(unsigned long x) {
    return __builtin_popcountl(x);
}
static inline int __rcc_stdc_count_ones_ull(unsigned long long x) {
    return __builtin_popcountll(x);
}

static inline unsigned int __rcc_stdc_bit_width_uc(unsigned char x) {
    return __rcc_uc_width - __rcc_stdc_leading_zeros_uc(x);
}
static inline unsigned int __rcc_stdc_bit_width_us(unsigned short x) {
    return __rcc_us_width - __rcc_stdc_leading_zeros_us(x);
}
static inline unsigned int __rcc_stdc_bit_width_ui(unsigned int x) {
    return __rcc_ui_width - __rcc_stdc_leading_zeros_ui(x);
}
static inline unsigned int __rcc_stdc_bit_width_ul(unsigned long x) {
    return __rcc_ul_width - __rcc_stdc_leading_zeros_ul(x);
}
static inline unsigned int __rcc_stdc_bit_width_ull(unsigned long long x) {
    return __rcc_ull_width - __rcc_stdc_leading_zeros_ull(x);
}

static inline int __rcc_stdc_trailing_ones_uc(unsigned char x) {
    /* trailing_ones(x) == trailing_zeros(~x); ~0 -> 0 -> width, all correct */
    return __rcc_stdc_trailing_zeros_uc((unsigned char)~x);
}
static inline int __rcc_stdc_trailing_ones_us(unsigned short x) {
    return __rcc_stdc_trailing_zeros_us((unsigned short)~x);
}
static inline int __rcc_stdc_trailing_ones_ui(unsigned int x) {
    return __rcc_stdc_trailing_zeros_ui(~x);
}
static inline int __rcc_stdc_trailing_ones_ul(unsigned long x) {
    return __rcc_stdc_trailing_zeros_ul(~x);
}
static inline int __rcc_stdc_trailing_ones_ull(unsigned long long x) {
    return __rcc_stdc_trailing_zeros_ull(~x);
}

#define stdc_leading_zeros_uc(x)  __rcc_stdc_leading_zeros_uc(x)
#define stdc_leading_zeros_us(x)  __rcc_stdc_leading_zeros_us(x)
#define stdc_leading_zeros_ui(x)  __rcc_stdc_leading_zeros_ui(x)
#define stdc_leading_zeros_ul(x)  __rcc_stdc_leading_zeros_ul(x)
#define stdc_leading_zeros_ull(x) __rcc_stdc_leading_zeros_ull(x)

#define stdc_trailing_zeros_uc(x)  __rcc_stdc_trailing_zeros_uc(x)
#define stdc_trailing_zeros_us(x)  __rcc_stdc_trailing_zeros_us(x)
#define stdc_trailing_zeros_ui(x)  __rcc_stdc_trailing_zeros_ui(x)
#define stdc_trailing_zeros_ul(x)  __rcc_stdc_trailing_zeros_ul(x)
#define stdc_trailing_zeros_ull(x) __rcc_stdc_trailing_zeros_ull(x)

#define stdc_count_ones_uc(x)  __rcc_stdc_count_ones_uc(x)
#define stdc_count_ones_us(x)  __rcc_stdc_count_ones_us(x)
#define stdc_count_ones_ui(x)  __rcc_stdc_count_ones_ui(x)
#define stdc_count_ones_ul(x)  __rcc_stdc_count_ones_ul(x)
#define stdc_count_ones_ull(x) __rcc_stdc_count_ones_ull(x)

#define stdc_trailing_ones_uc(x)  __rcc_stdc_trailing_ones_uc(x)
#define stdc_trailing_ones_us(x)  __rcc_stdc_trailing_ones_us(x)
#define stdc_trailing_ones_ui(x)  __rcc_stdc_trailing_ones_ui(x)
#define stdc_trailing_ones_ul(x)  __rcc_stdc_trailing_ones_ul(x)
#define stdc_trailing_ones_ull(x) __rcc_stdc_trailing_ones_ull(x)

#define stdc_bit_width_uc(x)  __rcc_stdc_bit_width_uc(x)
#define stdc_bit_width_us(x)  __rcc_stdc_bit_width_us(x)
#define stdc_bit_width_ui(x)  __rcc_stdc_bit_width_ui(x)
#define stdc_bit_width_ul(x)  __rcc_stdc_bit_width_ul(x)
#define stdc_bit_width_ull(x) __rcc_stdc_bit_width_ull(x)
#define stdc_leading_zeros(x) _Generic((x), \
    unsigned char:      __rcc_stdc_leading_zeros_uc, \
    unsigned short:     __rcc_stdc_leading_zeros_us, \
    unsigned int:       __rcc_stdc_leading_zeros_ui, \
    unsigned long:      __rcc_stdc_leading_zeros_ul, \
    unsigned long long: __rcc_stdc_leading_zeros_ull \
)(x)

#define stdc_trailing_zeros(x) _Generic((x), \
    unsigned char:      __rcc_stdc_trailing_zeros_uc, \
    unsigned short:     __rcc_stdc_trailing_zeros_us, \
    unsigned int:       __rcc_stdc_trailing_zeros_ui, \
    unsigned long:      __rcc_stdc_trailing_zeros_ul, \
    unsigned long long: __rcc_stdc_trailing_zeros_ull \
)(x)

#define stdc_count_ones(x) _Generic((x), \
    unsigned char:      __rcc_stdc_count_ones_uc, \
    unsigned short:     __rcc_stdc_count_ones_us, \
    unsigned int:       __rcc_stdc_count_ones_ui, \
    unsigned long:      __rcc_stdc_count_ones_ul, \
    unsigned long long: __rcc_stdc_count_ones_ull \
)(x)

#define stdc_trailing_ones(x) _Generic((x), \
    unsigned char:      __rcc_stdc_trailing_ones_uc, \
    unsigned short:     __rcc_stdc_trailing_ones_us, \
    unsigned int:       __rcc_stdc_trailing_ones_ui, \
    unsigned long:      __rcc_stdc_trailing_ones_ul, \
    unsigned long long: __rcc_stdc_trailing_ones_ull \
)(x)

#define stdc_bit_width(x) _Generic((x), \
    unsigned char:      __rcc_stdc_bit_width_uc, \
    unsigned short:     __rcc_stdc_bit_width_us, \
    unsigned int:       __rcc_stdc_bit_width_ui, \
    unsigned long:      __rcc_stdc_bit_width_ul, \
    unsigned long long: __rcc_stdc_bit_width_ull \
)(x)

#endif
