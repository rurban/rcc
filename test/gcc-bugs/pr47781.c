/* GCC Bug #47781 - warnings from custom printf format specifiers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47781
 */


#include <printf.h>

struct My {};
struct MyL {};
struct MyLL {};
__attribute__((printf_format_specifier(
//   "M",
//   /* pointer_count */ 1,
//   /* flag_chars    */ "-+0 #wp",
//   /* flags2        */ "iWR[]<>",
//   /* length token pairs */
//   "none", __typeof__(struct My *),
//   "l",    __typeof__(struct MyL *),
//   "ll",   __typeof__(struct MyLL *)
// )))
static int
// custom_arginfo_M (const struct printf_info *info, size_t n,
                  int *argtypes, int *size)
{
//   /* ... */
}

// /* ... */
register_printf_specifier ('M', custom_printf_M, custom_arginfo_M);

// This is a direct conversion from GCC internals, with the intention to support all mainstream printf single-character specifier cases, while leaving out things like Solaris-style multi-argument specs and the ability to extend existing specifiers. I also do not allow per-spec STD_* setting (STD_C89, STD_C99 etc), they stay at STD_EXT. So this proposal does not get full parity with all that format_char_info offers; it would just cover what I imagine to be the majority of cases based on reading uses in the wild.

// pointer_count (meaning the level of pointer indirection, with 0 meaning no indirection, 1 meaning one level, *p, and so on) applies uniformly to all length entries (%l, %ll, ..), there is not a per-length override.

// flag_chars and flags2 define what printf flags are supported by this new specifier. Both are validated against printf’s flag_specs; any character not defined there is rejected.

// For the length token pairs, "none" is a special value meaning format_lengths FMT_LEN_none signifying no length modifier (%M in this example). If "none" is not provided, then bare %M in this case would be disallowed. The other options are all matched against printf's format_kind_info.length_char_specs and translated by printf_length_specs. If we get a truly unknown token, that becomes a diagnostic.

// The gcc format_spec attribute can be set on the custom specifier's print function handler (custom_printf_M in this example) or on a dummy function if desired, it just needs to be available in the Translation Unit where the format checking is done by GCC.

// Happy to contribute an implementation (in the form of a plugin to start) and tests, just wanted to get alignment on the interface first.
// Thanks
// Gabriel


