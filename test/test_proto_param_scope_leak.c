/* Parser bug: a function declarator's parameter names (from either a
 * bare prototype or a full definition) leaked into the persistent
 * file-scope `locals` list and were never cleared once the declarator
 * finished. find_var() checks `locals` *before* falling back to
 * find_global_name()/enum lookups (so a real local always shadows a
 * same-named global, as C requires) -- so any later top-level
 * identifier sharing a name with an earlier prototype/definition
 * parameter silently resolved to that stale, wrongly-typed phantom
 * parameter instead of its own real declaration.
 *
 * Found via blosc2: a synthetic prelude function's `const char *f`
 * parameter shadowed a torture test's `enum K { ..., f, ... }`
 * constant, breaking its type and value everywhere `f` was referenced
 * afterward.
 */

/* Case 1: bare prototype (no body) leaves its param name behind. */
int proto1(const char *f);
enum K1 { e1 = 1,
          f,
          g1 = f }; /* `f` here must be the enum constant (value 2) */

/* Case 2: full definition leaves its param name behind too. */
static int proto2(const char *f) { return f ? 1 : 0; }
enum K2 { e2 = 1,
          f2,
          g2 = f2 };
int f2_user = f2; /* sanity: f2 itself must never have collided with anything */

int main(void) {
    if (g1 != 2) return 1;
    if (g2 != 2) return 2;
    if (f2_user != 2) return 3;
    return 0;
}
