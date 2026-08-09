/* On LP64 targets (native Linux/macOS x86-64 and arm64, where `long` is
 * 8 bytes), rcc's own <stdint.h> must define int64_t/uint64_t/intptr_t/
 * uintptr_t/intmax_t/uintmax_t as `long`/`unsigned long`, matching
 * glibc's own convention (bits/types.h's __int64_t/__intmax_t, guarded
 * on __WORDSIZE == 64) -- not merely "some 64-bit type", which the C
 * standard alone would allow but which conflicts with glibc's own
 * choice. A typedef name re-typedef'd to a DIFFERENT (if same-size)
 * underlying type is not the same type per C11 6.7p3: any TU pulling in
 * both this header (previously `long long`) and a real glibc header
 * re-declaring these names (extremely common -- countless system
 * headers transitively include <bits/stdint-intn.h> for __intN_t) hit a
 * real "conflicting types" error at every int64_t/intmax_t/intptr_t-
 * parametered function once the parser started diagnosing incompatible
 * redeclarations -- the same class of bug already documented and fixed
 * for ptrdiff_t.
 *
 * Found via test/third_party/test_libtommath:
 * `MP_INIT_INT(mp_init_i64, mp_set_i64, int64_t)`'s macro-expanded
 * definition disagreed with its own header-declared prototype once
 * glibc's <bits/types.h> (pulled in transitively) re-typedef'd int64_t
 * as `long` right after this header's own (then-`long long`) typedef
 * had already taken effect.
 */
#include <stdint.h>
#include <stdio.h>

#if !defined(_WIN32)
/* The actual bug shape: a prototype using the bare typedef name, later
 * "redeclared" (here, via a second typedef of the identical underlying
 * type -- the minimal reproduction of what a mixed rcc-stdint.h +
 * glibc-bits/types.h inclusion produces) must not conflict. */
typedef long int64_t_check;
_Static_assert(sizeof(int64_t) == sizeof(long), "int64_t must be long on LP64");
_Static_assert(sizeof(intptr_t) == sizeof(long), "intptr_t must be long on LP64");
_Static_assert(sizeof(intmax_t) == sizeof(long), "intmax_t must be long on LP64");

int64_t mp_get_i64(void);
void mp_set_i64(int64_t b);
int mp_init_i64(int64_t b);
#define MP_INIT_INT(name, set, type) \
    int name(type b) { set(b); return 0; }
MP_INIT_INT(mp_init_i64, mp_set_i64, int64_t)

static int64_t stored;
void mp_set_i64(int64_t b) { stored = b; }
int64_t mp_get_i64(void) { return stored; }
#endif

int main(void) {
#if !defined(_WIN32)
    if (mp_init_i64(42) != 0 || mp_get_i64() != 42) {
        printf("mp_init_i64/mp_get_i64 roundtrip failed\n");
        return 1;
    }
#endif
    printf("ok\n");
    return 0;
}
