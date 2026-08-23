/* Regression test: a function prototype declared against a bare forward
 * `enum tag;` placeholder (a GNU/C23 extension: an opaque forward
 * reference, later completed by a full `enum tag { ... };` elsewhere)
 * was wrongly rejected as "conflicting types" when the function's actual
 * DEFINITION appeared after the enum was completed.
 *
 * rcc's forward `enum tag;` handling only registered the tag for later
 * identity reuse when it carried a C23 fixed underlying type (`enum tag
 * : int;`); a bare `enum tag;` (no `: type`) did not, so when the body
 * eventually completed the enum, it minted an unrelated Type object
 * (different enum_id) instead of completing the placeholder in place.
 * The prototype's parameter type (still pointing at the placeholder) and
 * the definition's parameter type (the freshly completed, unrelated
 * Type) then looked incompatible.
 *
 * Real-world impact: GNU make's src/makeint.h declares `enum
 * variable_origin;` and `void reset_makeflags(enum variable_origin);`
 * before src/variable.h completes `enum variable_origin { ... };`; the
 * definition in src/main.c failed to compile.
 */

enum color; /* bare forward declaration, no body yet */

void set_color(enum color c);

enum color { RED, GREEN, BLUE };

int last_color = -1;

void set_color(enum color c) { /* must NOT be "conflicting types" */
    last_color = (int)c;
}

int main(void) {
    set_color(GREEN);
    if (last_color != GREEN) return 1;
    return 0;
}
