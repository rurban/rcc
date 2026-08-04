/* GCC Bug #63303 - Pointer subtraction is broken when using -fsanitize=undefined
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63303
 */


#include <stdio.h>
#include <stddef.h>

// __attribute((noinline,noclone)) ptrdiff_t ptr_diff(char *p1, char *p2)
{
        return p1 - p2;
}

// __attribute((noinline,noclone)) void *ptr_add(char *p1, unsigned long p2)
{
        return p1 + p2;
}

void *get_address(unsigned n)
{
        return (void *)((unsigned long)n << (sizeof(void *) * 8 - 4));
}

int main(void)
{
//         printf("%ld\n", (long)ptr_diff(get_address(0x9), get_address(0x7))); /* sanitizer should not warn here */
//         printf("%ld\n", (long)ptr_diff(get_address(0xc), get_address(0x3))); /* sanitizer should warn here */
        return 0;
}


