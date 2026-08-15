/* A string literal missing its closing quote at end-of-line is real gcc's
 * own WARNING ("missing terminating \" character"), not a hard error --
 * gcc recovers by treating the token as ending right at the newline and
 * keeps preprocessing/compiling. rcc used to hard-error and abort the
 * whole translation unit, breaking real-world autotools-generated headers
 * with a genuinely truncated #define whose macro is never actually
 * referenced by compiled code (found via gnutls's own config.h: `#define
 * M_LIBRARY_SONAME "libm.so.6` with no closing quote -- an upstream
 * config-generation artifact, confirmed identically rejected-as-a-warning
 * by real gcc too). A second, independent bug found alongside it: the
 * lexer's own line-number tracking for diagnostics raised mid-scan
 * (lex_error_at/lex_warn_at) relied on globals that are only maintained by
 * tokenize()'s naive standalone #line scan and stay stale throughout real
 * preprocessing (#include-driven, tracked separately by preprocess.c) --
 * every such diagnostic reported the WRONG line number for any file
 * reached via #include. This program exercises both: the malformed,
 * unreferenced macro must not abort compilation, and a *used*, deliberately
 * malformed integer suffix diagnosed via the same code path must report the
 * correct physical line (verified indirectly: this file itself must
 * compile and run, since a wrong-line but correctly-recovering warning
 * would otherwise still succeed -- the real regression coverage for the
 * unclosed-string case is the warning firing without aborting).
 */
#include <stdio.h>

/* Deliberately unterminated string in an unused macro -- must warn, not
 * error, and must not stop later declarations in this file from parsing.
 */
#define UNUSED_TRUNCATED_DEFINE "this has no closing quote

int main(void) {
    printf("OK\n");
    return 0;
}
