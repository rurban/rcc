/* GCC Bug #125995 - constexpr struct initialization leads to unexpected results
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125995
 */


#define BAR_H

struct Foo {
    int baz;
    int qux;
};

struct Bar {
    struct Foo foo;
};

const struct Bar* get_bar();

#endif // BAR_H
// ```
// bar.c:
// ```
#include "bar.h"
static constexpr struct Bar b = {
    .foo = {
        .baz = 1
    }
};

const struct Bar* get_bar() {
    struct Foo foo = b.foo;
    return &b;
}
// ```
// main.c:
// ```
#include "bar.h"
int main() {
    int a = 1;
    int diff = __builtin_memcmp(&get_bar()->foo.baz, &a, sizeof(a));
    if (diff)
        __builtin_abort();
    return 0;
}
// ```
// Compiled with `gcc -std=c23 -O2 main.c bar.c`.


