/* A compound literal nested in another struct compound literal whose array
 * type has a non-literal dimension containing runtime operands:
 *   (struct T){ (var[sizeof((var[]){mk(1), x})/sizeof(var) + 1]){0} }
 * The speculative constant-fold scan skipped the whole parenthesized type
 * group, saw only `{0}`, and tried to fold the literal as static data; the
 * dimension's `{mk(1), x}` then hard-errored ("expected specific operator").
 * Cello's foreach(pair in enumerate(x)) expands to exactly this shape.
 */
#include <stdio.h>

typedef void *var;
struct T { var *v; };

static var mk(int n) { return (var)(long)n; }

static int f(var x) {
    struct T t = (struct T){(var[(sizeof((var[]){mk(1), x}) / sizeof(var)) + 1]){0}};
    return t.v != 0 && t.v[0] == 0 && t.v[2] == 0 ? (int)(sizeof((var[]){mk(1), x}) / sizeof(var)) : -1;
}

int main(void) {
    if (f(0) != 2) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
