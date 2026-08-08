// C11 6.2.1p4: an enum (or struct/union tag, or enum constant) declared
// inside a function's parameter-type-list has "function prototype scope" --
// visible only within that parameter list and (for a definition) the
// function body, then reverting to whatever was visible before at file
// scope. rcc's enum-constant table (enum_consts/enum_htab) had no scope
// tracking at all around declarator_params(): an enum declared inside a
// parameter list registered permanently into the flat file-scope table and
// was never popped, so it silently shadowed the real file-scope enumerator
// of the same name for the rest of the translation unit.
//
// Mirrors michaelforney/cproc's test/func-param-scope.c.
#include <assert.h>

enum { A = 1 };

// `enum { A = 2 }` here has scope limited to this parameter list plus the
// function body; the array-length uses of A below must see A == 2, but the
// file-scope A must be restored to 1 immediately after the closing brace.
char (*f(enum { A = 2 } *p, int (*a)[A]))[A] {
    static_assert(A == 2, "param-list enum visible inside body");
    static_assert(sizeof *a == 2 * sizeof(int), "param-list enum usable in a sibling param's array length");
    (void)p;
    return 0;
}

// A bare prototype (no body) must not leak its parameter-list enum either.
int g(enum { B = 99 } *q);

static_assert(A == 1, "file-scope A must be restored after the function definition");

int main(void) {
    assert(A == 1);
    assert(f(0, 0) == (void *)0);
    return 0;
}
