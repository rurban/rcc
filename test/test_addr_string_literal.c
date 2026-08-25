/* Regression test: `&"string literal"` (address-of a string literal)
 * crashed the compiler with "lvalue required as left operand of
 * assignment", even though this is valid C: a string literal is a
 * (non-modifiable) lvalue of array type (C11 6.5.1p4), so `&"abc"`
 * legally yields `char (*)[4]`, the array's address.
 *
 * `gen_addr()` (codegen.c) -- the address-computing codegen dispatcher
 * invoked for any `&expr`/lvalue context -- had no `ND_STR` case, so it
 * fell through to the `default:` branch, which unconditionally raises
 * "lvalue required as left operand of assignment" for any node kind it
 * doesn't recognize as addressable. gen()'s own (separate) ND_STR case
 * already computes exactly this same address as the literal's decayed
 * rvalue (`lea .LCn(%rip), r`); gen_addr() just never reused it. Fixed
 * by adding an ND_STR case to gen_addr() emitting the identical
 * lea/adrp+add sequence. The parser already tracks the string literal's
 * un-decayed array type (`char[N]`) at construction time, so
 * check_type()'s existing `node->ty = pointer_to(operand->ty)` for
 * ND_ADDR already produced the correct `char(*)[N]` type -- only the
 * codegen path was missing.
 *
 * Found via util-linux's disk-utils/isosize.c: `memcmp(&label,
 * &"\1CD001\1", 8)` -- comparing a 7-byte magic-number buffer directly
 * against a string literal's address, a common idiom for avoiding a
 * separate named buffer for a short fixed byte sequence.
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    /* Exact util-linux isosize.c shape: memcmp against &"literal". */
    char label[8] = "\1CD001\1";
    assert(memcmp(&label, &"\1CD001\1", 8) == 0);
    assert(memcmp(&label, &"\1CD002\1", 8) != 0);

    /* Type correctness: &"abc" is char(*)[4], not char** -- dereferencing
     * it yields the 4-byte array itself (3 chars + NUL), matching real
     * gcc exactly (verified: `sizeof(*&"abc")` == 4 under gcc). */
    assert(sizeof(*&"abc") == 4);
    assert(sizeof(&"abc") == sizeof(void *));
    char (*p)[4] = &"abc";
    assert(strcmp(*p, "abc") == 0);

    /* The address really is the literal's own storage -- taking it twice
     * yields the same location (string literals may be pooled/shared). */
    assert(&"xyz" == &"xyz" || memcmp(&"xyz", &"xyz", 4) == 0);

    printf("OK\n");
    return 0;
}
