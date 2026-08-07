/* Compile-time constant folding of a floating-point subexpression cast
 * to an integer type. Found via flatcc's refmap.c:
 *
 *   static const size_t n = (size_t)((0.7f) * 256.0f);
 *
 * eval_const_expr()'s ND_FNUM case truncated each float literal to an
 * integer *before* combining them via ND_MUL (0.7f -> (long long)0,
 * 256.0f -> 256), folding `0.7f * 256.0f` to 0 * 256 = 0 instead of
 * (long long)(0.7f * 256.0f) = 179. That made flatcc's hash-table
 * load-factor threshold permanently 0, so
 * `_flatcc_refmap_above_load_factor()` was always true and its resize
 * loop's `buckets *= 2` never terminated -- doubling forever (and
 * eventually wrapping to 0), spinning at 100% CPU the first time the
 * table needed to grow. Every floating subexpression feeding an
 * integer cast in a constant context (static/global initializers,
 * array sizes, enum values, _Static_assert) must be folded in floating
 * point throughout and converted to an integer only at the cast. */

static const int a = (int)(0.7f * 256.0f);           /* 179, not 0 */
static const int b = (int)(1.5 + 2.5);                /* 4 */
static const int c = (int)(10.0 / 4.0);               /* 2 */
static const int d = (int)(3.9);                      /* 3: truncation, not rounding */
static const int e = (int)(-1.5 * 2.0);                /* -3 */
static const unsigned int f = (unsigned int)(2.0 * 2.0); /* 4 */

/* Array size computed via a float-to-int constant cast must also fold
 * correctly (a wrong fold here would be a compile error or wrong
 * sizeof, not just a runtime surprise). */
static const char buf[(int)(4.0 * 2.0)]; /* size 8 */

int main(void) {
    if (a != 179) return 1;
    if (b != 4) return 2;
    if (c != 2) return 3;
    if (d != 3) return 4;
    if (e != -3) return 5;
    if (f != 4) return 6;
    if (sizeof(buf) != 8) return 7;
    return 0;
}
