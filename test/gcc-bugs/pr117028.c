/* GCC Bug #117028 - [C2y] Implement N3353, Obsolete implicitly octal literals and add delimited escape sequences
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117028
 */


// (\u{123}, \x{123}, \o{123}) which were added already for C++23,
//     (\N{LATIN CAPITAL LETTER C WITH CARON}), which C++23 has but C2Y doesn't
            CPP_OPTION (pfile, delimited_escape_seqs) in \N{} related tests.
//             Change wording of C cpp_pedwarning for \u{} and emit
//             (convert_hex): Change wording of C cpp_pedwarning for \u{} and emit


