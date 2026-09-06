/* GCC Bug #114713 - incorrect TBAA for struct with flexible array member or GNU zero size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114713
 */
/* { dg-do compile } */

/* NOTE: genuinely multi-TU (LTO TBAA wrong-code): one TU defines
 * `struct foo { int x; char a[]; }` (standard FAM), the other the
 * transition-era `struct foo { int x; char a[0]; }` (GNU zero-size).
 * gcc's LTO TBAA treats the two as incompatible for aliasing even though
 * code migrating between the forms relies on them aliasing, producing
 * wrong code.  A single TU cannot hold both definitions, so TU1 is kept
 * here as a reference (comment 0 of the bug). */
struct foo { int x; char a[]; };

void test_bar(void *b);

__attribute__((noinline))
int test_foo(struct foo *a, void *b)
{
    a->x = 1;
    test_bar(b);
    return a->x;
}

int main(void)
{
    struct foo y;
    return test_foo(&y, &y) == 1 ? 0 : 1;
}
