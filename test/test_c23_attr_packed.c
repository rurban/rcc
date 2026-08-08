// C23 `[[gnu::packed]]` / `[[__gnu__::packed]]` double-bracket attribute
// syntax placed between the struct/union keyword and the tag name must have
// the exact same packing effect as the legacy GNU
// `__attribute__((packed))` syntax. rcc's read_type_attrs() already parsed
// `[[...]]` attribute lists, but for a namespaced attribute like `gnu::` it
// only skipped past the `namespace::` prefix and then failed to recognize
// the attribute name that followed, silently discarding it instead of
// setting the packed state that feeds struct/union member-layout
// computation (struct_pack).
//
// Mirrors michaelforney/cproc's test/union-packed.c.
#include <stddef.h>

union [[gnu::packed]] u1 {
    int x;
    char y[5];
};
union u2 {
    int x;
    char y[5];
};
static_assert(sizeof(union u1) == 5, "[[gnu::packed]] union must drop trailing padding");
static_assert(sizeof(union u2) == 8, "unpacked union keeps natural alignment padding");

struct [[gnu::packed]] s1 {
    char a;
    int b;
};
static_assert(sizeof(struct s1) == 5, "[[gnu::packed]] struct must close internal padding");
static_assert(offsetof(struct s1, b) == 1, "[[gnu::packed]] struct must tightly pack member offsets");

// The `__gnu__::` reserved-identifier spelling of the namespace must work too.
struct [[__gnu__::packed]] s2 {
    char a;
    int b;
};
static_assert(sizeof(struct s2) == 5, "[[__gnu__::packed]] must pack identically to [[gnu::packed]]");

int main(void) {
    struct s1 v = {(char)7, 99};
    if (v.a != 7 || v.b != 99)
        return 1;
    return 0;
}
