/* A trailing `_Pragma(...)` (or `__attribute__`/`[[...]]`) immediately
 * before a struct/union's closing `}`, with no further member after
 * it, was mishandled by the struct-member-list parser -- e.g. glib's
 * own idiom in <gio/gdtlsconnection.h>:
 *
 *   struct _GDtlsConnectionInterface {
 *     ...
 *     G_GNUC_BEGIN_IGNORE_DEPRECATIONS
 *       gboolean (*get_binding_data) (...);
 *     G_GNUC_END_IGNORE_DEPRECATIONS
 *   };
 *
 * where G_GNUC_END_IGNORE_DEPRECATIONS expands to
 * `_Pragma("GCC diagnostic pop")` with nothing else before the "};".
 *
 * The member-list loop unconditionally called declspec() at the top of
 * every iteration; declspec() consumed the trailing _Pragma via
 * read_type_attrs(), found no real type keyword left before "}",
 * silently fell back to implicit int (C89-style), and returned with
 * tok still pointing at "}" -- which the surrounding member-parsing
 * code then handed to declarator() as if "}" could start a member
 * name, producing "expected specific operator". Found via test_emacs
 * (xterm.h's `extern void x_scroll_bar_configure (GdkEvent *);`,
 * behind `#ifdef HAVE_GTK3`, pulling in this exact glib header
 * transitively) and test_liballegro5 (same header, same shape).
 *
 * Fixed by peeking (non-destructively) past any leading attrs/pragmas
 * at the top of each member-list iteration; only actually consuming
 * them there -- and ending the loop -- when nothing but "}" follows. A
 * real member's own leading attribute (e.g. `alignas(128) int x;`)
 * must still reach declspec() untouched so its alignment is recorded
 * normally; an earlier, over-eager version of this fix broke exactly
 * that case (regressed GCC torture c23-tag-9.c/c23-tag-composite-6.c),
 * caught during verification and fixed with the peek-first approach
 * before landing.
 */

struct trailing_pragma_only {
    int a;
    int b;
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wunused-function\"")
};

struct trailing_pragma_with_member_after {
    int a;
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wunused-function\"")
    int b;
_Pragma("GCC diagnostic pop")
};

/* A real leading member attribute must still work correctly (not
 * silently discarded by the trailing-pragma peek). */
struct alignas_member {
    _Alignas(64) int x;
    int y;
};

int main(void)
{
    struct trailing_pragma_only s1 = {1, 2};
    if (s1.a + s1.b != 3) return 1;

    struct trailing_pragma_with_member_after s2 = {1, 2};
    if (s2.a + s2.b != 3) return 2;

    if (_Alignof(struct alignas_member) != 64) return 3;

    return 0;
}
