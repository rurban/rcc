/* GCC Bug #117028 - [C2y] Implement N3353, Obsolete implicitly octal literals and add delimited escape sequences
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117028
 */
/* { dg-do compile } */
/* { dg-options "-std=c2y" } */

// This PR requested implementing WG14 N3353 for C2y: obsolete implicitly
// octal literals (0123) in favour of a new 0o/0O prefix, and add delimited
// escape sequences (\u{123}, \x{123}, \o{123}) that C++23 already had.
// The patch committed for this PR (r15-4403-ge020116) implements the new
// 0o/0O octal-literal syntax and the delimited escape sequences for C, but
// deliberately does *not* add any obsolescence diagnostic for the old
// \123 / 0123 forms (left for a possible later, non-default warning).

_Static_assert (0o173 == 0173, "0o173 octal literal matches 0173");
_Static_assert ('\x{7a}' == 'z', "\\x{} delimited hex escape sequence");
_Static_assert ('\o{172}' == 'z', "\\o{} delimited octal escape sequence");

// No diagnostic is emitted for the still-legal old-style octal literal or
// escape sequence below, matching the "no obsolescence diagnostics" outcome
// discussed and agreed upon in this PR.
int old_style_octal = 0173;
char old_style_escape = '\172';
