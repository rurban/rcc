/* C11 6.7.5: an alignment-specifier is only rejected as part of a plain
 * type-name (e.g. directly inside a cast or sizeof(TYPE)); it is always
 * legal as part of a struct/union MEMBER declaration, per 6.7.2.1p1's
 * ordinary struct-declaration grammar -- even when that struct/union body
 * is itself written inline as the type-name operand of sizeof()/
 * _Alignof()/alignof(). rcc's parser set an `in_type_name` flag for the
 * whole duration of parsing a type-name (including any nested struct/
 * union body within it) and rejected any `alignas`/`_Alignas` seen while
 * that flag was set -- wrongly firing on a perfectly legal member
 * declaration nested inside the type-name's own struct/union specifier,
 * not just a genuine `alignas(N) int` type-name written bare.
 *
 * Found via GNU Emacs's src/alloc.c:
 *   enum { LISP_ALIGNMENT = alignof (union { union emacs_align_type x;
 *                                            char alignas (GCALIGNMENT) gcaligned; }) };
 * which failed with "alignment specified for type name" and then a
 * cascade of "undeclared variable" errors for every later use of the
 * enum constant it was supposed to define, aborting the whole build.
 */
#include <stdalign.h>
#include <stddef.h>

/* Minimal repro matching alloc.c's exact shape: alignas on a member
 * inside an anonymous union used as _Alignof's type-name operand. */
enum { EMACS_ALIGN = _Alignof(union { long x; char alignas(16) gcaligned; }) };

/* Also verify the alignment actually took effect (not just parsed). */
struct S {
    char c;
    alignas(32) int i;
};

/* Alignas is still correctly REJECTED when it appears directly on a bare
 * type-name (not as part of a nested member declaration) -- e.g. a cast
 * or a plain sizeof(TYPE) operand. Compile-time-only check: if this ever
 * regresses to being silently accepted, it would only show up as a
 * -pedantic-errors compile failure elsewhere, so this file intentionally
 * does NOT try to construct that case (it would itself be an error and
 * break this test file's own compile). */

int main(void)
{
    if (EMACS_ALIGN != 16) return 1;
    if (_Alignof(struct S) != 32) return 2;
    if (offsetof(struct S, i) != 32) return 3;
    return 0;
}
