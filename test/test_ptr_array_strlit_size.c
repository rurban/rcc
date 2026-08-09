/* A single-element array-of-pointers initializer using one string literal,
 * `T *arr[] = { "literal" };`, must size the array by its ONE initializer
 * ELEMENT (a pointer), not by the string literal's own length. That
 * strlen-based sizing is only correct for C11 6.7.9p14's actual case: an
 * array of CHARACTER type initialized (optionally brace-wrapped) by a
 * character string literal, e.g. `char buf[] = { "hello" };`.
 *
 * infer_array_type() (parser.c) unwrapped `{ STRLIT }` and sized the array
 * from strlen()+1 unconditionally, with no check that the array's element
 * type was actually a character type -- so `const char *arr[] = { "vec_" }`
 * (a correctly-typed ONE-pointer array, the string literal merely supplying
 * the address arr[0] points at) got sized as 5 elements (strlen("vec_")+1)
 * instead of the correct 1, leaving 4 phantom trailing pointer slots
 * pointing at whatever adjacent .rodata/unmapped memory happened to follow.
 *
 * Found via test/third_party/test_flatcc's gperf-generated
 * fb_reserved_kw_vec_prefixes[] = { "vec_" }: a loop walking that array by
 * its (wrongly inferred) length ran 4 iterations past the real end and
 * crashed calling strlen() on garbage.
 */
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static const char *one[] = {"a"};
static const char *three[] = {"vec_"};

/* Narrow and wide character arrays must still be sized by the string's own
 * length -- the actual C11 6.7.9p14 case this fix must not regress. */
static char narrow[] = {"hello"};
static wchar_t wide[] = L"hi";

int main(void) {
    if (sizeof(one) / sizeof(one[0]) != 1) {
        printf("sizeof(one)/sizeof(one[0]) = %d, expected 1\n",
               (int)(sizeof(one) / sizeof(one[0])));
        return 1;
    }
    if (sizeof(three) / sizeof(three[0]) != 1) {
        printf("sizeof(three)/sizeof(three[0]) = %d, expected 1\n",
               (int)(sizeof(three) / sizeof(three[0])));
        return 2;
    }
    if (strcmp(one[0], "a") != 0 || strcmp(three[0], "vec_") != 0) {
        printf("wrong pointer contents\n");
        return 3;
    }
    if (sizeof(narrow) != 6) {
        printf("sizeof(narrow) = %d, expected 6\n", (int)sizeof(narrow));
        return 4;
    }
    if (sizeof(wide) / sizeof(wide[0]) != 3) {
        printf("sizeof(wide)/sizeof(wide[0]) = %d, expected 3\n",
               (int)(sizeof(wide) / sizeof(wide[0])));
        return 5;
    }
    printf("ok\n");
    return 0;
}
