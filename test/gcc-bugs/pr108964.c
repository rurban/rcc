/* GCC Bug #108964 - Attribute (multi-) target should be implemented for C front-end (not only c++)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108964
 */


__attribute__((target("default")))
int test_fn()
{
    return 1;
}

__attribute__((target("sse2")))
int test_fn()
{
    return 0;
}
int main()
{
    return test_fn();
}

// Bastien (rouca for debian)


