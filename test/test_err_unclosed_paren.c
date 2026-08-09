/* An unclosed '(' inside a declarator must be a clean parse error, not a
 * crash. declarator()'s nested/grouped-paren disambiguation (C11 6.7.6p3)
 * scans forward for the matching ')' and, on finding one, recurses into
 * declarator() again starting just past the opening '('. When the input
 * runs out before a matching ')' is ever found, that scan silently stops
 * at TK_EOF and the recursive call was handed TK_EOF as its own `tok` --
 * whose `->next` is NULL (the lexer's genuine end-of-token-list sentinel,
 * never linked further). declarator()'s very next statement unconditionally
 * dereferences `tok->next`, so that NULL flows straight into
 * skip_attributes()/read_type_attrs(), which dereferences `tok->kw` and
 * segfaults.
 *
 * Found via ksh93's own AT&T ast-open build-configury (src/cmd/INIT/probe),
 * which deliberately compiles a single bare '(' as a "this must fail to
 * compile" negative-compilation-check -- expecting a diagnostic, not a
 * SIGSEGV, so the probe script itself could tell the two apart.
 */
(
