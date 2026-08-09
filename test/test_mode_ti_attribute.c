/* __attribute__((mode(TI))) requests a 128-bit integer type -- GCC's
 * mode-attribute machinery, one tier past the QI/HI/SI/DI (1/2/4/8-byte)
 * modes rcc already supported. Two separate bugs, both found via
 * test/third_party/test_libtommath (`typedef unsigned long mp_word
 * __attribute__((mode(TI)));`, gated on rcc's own predefined macros
 * correctly selecting libtommath's MP_64BIT configuration):
 *
 * 1. TI itself was entirely unrecognized -- the mode-attribute parser only
 *    matched QI/HI/SI/DI, so `mode(TI)` silently did nothing and mp_word
 *    stayed 8 bytes (an ordinary `unsigned long`) instead of 16.
 *
 * 2. Once TI was added, a second, pre-existing bug surfaced: a *trailing*
 *    mode() attribute -- written after the declarator's identifier, GCC's
 *    own and libtommath's actual convention (`typedef T name
 *    __attribute__((mode(...)));`) -- was parsed (setting the internal
 *    pending_mode flag as a side effect) but never *applied* to the
 *    identifier's own type at that point in declarator(); only a
 *    *leading*-position mode() (`T __attribute__((mode(...))) name;`) was
 *    consumed. The flag leaked, global and unreset, into whatever
 *    declarator() call ran next -- silently resizing/retyping the
 *    *following*, unattributed declaration instead of the one that
 *    actually carried the attribute.
 */
#include <stdio.h>

typedef unsigned long mp_word_u __attribute__((mode(TI)));
typedef long mp_word_s __attribute__((mode(TI)));

/* Deliberately declared right after the typedefs above: with bug #2, the
 * leaked pending_mode from mp_word_s's trailing attribute would have
 * silently widened THIS unrelated int to 128 bits instead. */
int not_ti = 42;

int main(void) {
    if (sizeof(mp_word_u) != 16 || sizeof(mp_word_s) != 16) {
        printf("sizeof(mp_word_u)=%d sizeof(mp_word_s)=%d, expected 16 both\n",
               (int)sizeof(mp_word_u), (int)sizeof(mp_word_s));
        return 1;
    }
    if (sizeof(not_ti) != sizeof(int)) {
        printf("sizeof(not_ti)=%d, expected %d -- pending_mode leaked\n",
               (int)sizeof(not_ti), (int)sizeof(int));
        return 2;
    }

    /* Real 128-bit arithmetic, not just a resized 64-bit type: shifting
     * past bit 63 must actually carry into the upper half. */
    mp_word_u a = 1;
    a <<= 100;
    if ((unsigned long long)(a >> 64) != (1ULL << 36)) {
        printf("got %llu, expected %llu\n", (unsigned long long)(a >> 64), 1ULL << 36);
        return 3;
    }

    /* Signedness must be preserved from the base type, not hardcoded. */
    mp_word_s neg = -1;
    if (neg >= 0) {
        printf("mp_word_s lost its signedness\n");
        return 4;
    }

    printf("ok\n");
    return 0;
}
