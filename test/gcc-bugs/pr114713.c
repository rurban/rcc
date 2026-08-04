/* GCC Bug #114713 - incorrect TBAA for struct with flexible array member or GNU zero size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114713
 */


struct foo { int x; char a[]; };

void test_bar(void* b);

__attribute__((noinline))
int test_foo(struct foo* a, void* b)
{
//         a->x = 1;
//         test_bar(b);
        return a->x;
}

int main()
{
        struct foo y;

        if (2 != test_foo(&y, &y))
                __builtin_abort();

        return 0;
}

// TU2
struct foo { int x; char a[0]; };

void test_bar(void* b)
{
        struct foo *p = b;
//         p->x = 2;
}


