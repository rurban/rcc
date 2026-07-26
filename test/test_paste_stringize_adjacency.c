/* The C preprocessor's "#" stringize operator must reproduce whatever
 * whitespace actually separated two tokens in the macro's own body text —
 * if a "##"-pasted token was immediately followed by another token with no
 * space between them in the definition, stringizing that expansion must
 * keep them touching too. rcc's stringize_list()/str_needs_space() decided
 * adjacency purely from comparing each token's raw source-buffer pointer to
 * its neighbor's, but a "##" paste always synthesizes a brand-new token by
 * re-lexing into a *freshly allocated* buffer — so it could never appear
 * pointer-adjacent to the following token even when the original body had
 * zero whitespace between them, and a spurious space got inserted every
 * time.
 *
 * Found via a real Linux kernel build: include/linux/export.h's
 * EXPORT_SYMBOL machinery expands to
 * "asm(__stringify(___EXPORT_SYMBOL(sym, license, ns)))", whose body defines
 * "__export_symbol_##sym:" with no space before the colon. Stringizing that
 * (to hand the whole thing to the assembler as one asm() string) turned it
 * into "__export_symbol_do_exit :" — with a spurious space — which rcc's
 * *own* assembler then failed to parse as a label definition at all,
 * reporting "warning: unknown x86 instruction: __export_symbol_do_exit"
 * and silently dropping the exported-symbol table entry.
 */
#define PASTE_COLON(sym) sym##_tag:
#define STRINGIFY_1(x) #x
#define STRINGIFY(x) STRINGIFY_1(x)

int main(void)
{
    const char *s = STRINGIFY(PASTE_COLON(foo));
    /* Must be "foo_tag:" (no space before ':'), matching what real cpp
     * produces — not "foo_tag :". */
    const char *p = s;
    while (*p && *p != ':') p++;
    if (*p != ':') return 1; /* colon missing entirely */
    if (p != s && p[-1] == ' ') return 2; /* spurious space before ':' */
    if (p[-1] != 'g') return 3; /* sanity: should directly follow "...tag" */
    return 0;
}
