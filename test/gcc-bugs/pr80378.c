/* GCC Bug #80378 - Extend alloc_size attribute for better Linux kernel checking
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80378
 */


extern void *do_alloc(int a, int b);

static inline __attribute__((alloc_size(1))) void check_alloc_size(int size)
{
}

static inline void *alloc(int a, int b)
{
//         check_alloc_size(a + b);
        return do_alloc(a, b);
}

void func(void)
{
//         alloc(-1, 0);
}


