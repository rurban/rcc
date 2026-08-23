/* rcc's bundled <stdint.h> must fully conform to C99 7.18: every
 * required limit macro must be present, and every MIN/MAX macro's
 * own type must match its corresponding typedef's actual type.
 *
 * Found via ggrep (GNU grep) 3.12's own `./configure` gnulib probe
 * "checking whether stdint.h conforms to C99": it failed against rcc
 * for two independent reasons, causing gnulib to substitute its own
 * (partly UB-laden, in a rarely-exercised branch) <stdint.h>
 * replacement for every rcc-built project -- a real, observable
 * difference from building the identical source with GCC/Clang, where
 * the probe passes and gnulib's replacement is never even generated.
 *
 * 1. WCHAR_MIN/WCHAR_MAX (and WINT_MIN/MAX, SIG_ATOMIC_MIN/MAX) were
 *    missing entirely, even though the compiler already predefines the
 *    underlying __WCHAR_MIN__/__WCHAR_MAX__ etc. (matching GCC's own
 *    convention) -- <stdint.h> just never exposed them under their
 *    standard names.
 *
 * 2. INT64_MAX/UINT64_MAX (and everything built from them: SIZE_MAX,
 *    INTPTR_MAX, PTRDIFF_MAX, INTMAX_MAX, UINTMAX_MAX) used a fixed
 *    `LL`/`ULL` literal suffix regardless of target, while int64_t/
 *    uint64_t/intmax_t/uintmax_t/size_t are typedef'd as plain `long`/
 *    `unsigned long` on LP64 (Linux/macOS) and only `long long`/
 *    `unsigned long long` on LLP64 (Windows/mingw). The mismatch meant
 *    `_Generic(SIZE_MAX, size_t: ...)` had no matching association on
 *    Linux/macOS, even though SIZE_MAX's numeric *value* was correct.
 */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

/* Bug 1: the limit macros must exist and be usable in a constant
 * expression (a plain #ifdef check, like gnulib's own probe, plus an
 * actual numeric sanity check). */
#ifndef WCHAR_MIN
#error "WCHAR_MIN missing from <stdint.h>"
#endif
#ifndef WCHAR_MAX
#error "WCHAR_MAX missing from <stdint.h>"
#endif
#ifndef WINT_MIN
#error "WINT_MIN missing from <stdint.h>"
#endif
#ifndef WINT_MAX
#error "WINT_MAX missing from <stdint.h>"
#endif
#ifndef SIG_ATOMIC_MIN
#error "SIG_ATOMIC_MIN missing from <stdint.h>"
#endif
#ifndef SIG_ATOMIC_MAX
#error "SIG_ATOMIC_MAX missing from <stdint.h>"
#endif
_Static_assert(WCHAR_MIN < WCHAR_MAX, "WCHAR_MIN/MAX form a real range");
_Static_assert(WINT_MIN <= WINT_MAX, "WINT_MIN/MAX form a real range");
_Static_assert(SIG_ATOMIC_MIN < SIG_ATOMIC_MAX, "SIG_ATOMIC_MIN/MAX form a real range");

/* Bug 2: every *_MAX macro built on top of int64_t/uint64_t's literal
 * suffix must have EXACTLY the same type as its corresponding typedef
 * -- this is what gnulib's own probe checks via _Generic, and what
 * silently broke before the fix (values were already numerically
 * correct; only the TYPE disagreed). */
_Static_assert(_Generic(SIZE_MAX, size_t: 1, default: 0), "SIZE_MAX has type size_t");
_Static_assert(_Generic(UINT64_MAX, uint64_t: 1, default: 0), "UINT64_MAX has type uint64_t");
_Static_assert(_Generic(INT64_MAX, int64_t: 1, default: 0), "INT64_MAX has type int64_t");
_Static_assert(_Generic(INTMAX_MAX, intmax_t: 1, default: 0), "INTMAX_MAX has type intmax_t");
_Static_assert(_Generic(UINTMAX_MAX, uintmax_t: 1, default: 0), "UINTMAX_MAX has type uintmax_t");
_Static_assert(_Generic(PTRDIFF_MAX, ptrdiff_t: 1, default: 0), "PTRDIFF_MAX has type ptrdiff_t");

int main(void) {
    return 0;
}
