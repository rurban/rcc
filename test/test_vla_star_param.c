/* A VLA array-parameter whose size expression begins with a unary '*'
 * (a pointer dereference), e.g. `int a[*n]`, was misparsed: type_suffix()
 * treated the '*' as the C99 [*] unspecified-size marker and then hit
 * "expected specific operator" on the rest. This is exactly what munit's
 * MUNIT_ARRAY_PARAM(*params_size) expands to under GNU C. The [*] marker
 * itself (in a prototype) must still be accepted. */
#include <stddef.h>

/* [*] unspecified-size VLA marker in a prototype: still valid. */
int proto(size_t n, int a[*]);

/* [*n] VLA-sized parameter in a definition (decays to a pointer; the
 * size expression is a pointer dereference). */
static int sum(size_t *n, int a[*n])
{
    int s = 0;
    for (size_t i = 0; i < *n; i++)
        s += a[i];
    return s;
}

int main(void)
{
    int a[4] = {1, 2, 3, 4};
    size_t n = 4;
    return sum(&n, a) == 10 ? 0 : 1;
}
