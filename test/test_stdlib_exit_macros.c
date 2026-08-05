/* <stdlib.h> must define the standard EXIT_SUCCESS / EXIT_FAILURE macros
 * (C89 7.20) and RAND_MAX. rcc's bundled stdlib.h lacked them, so programs
 * using `exit(EXIT_SUCCESS)` or `rand()/RAND_MAX` failed with "undeclared
 * variable" (darkhttpd, genann, heatshrink, munit). RAND_MAX must equal
 * the glibc value rcc links against, else code scaling rand() by RAND_MAX
 * goes out of range. */
#include <stdlib.h>

int main(void)
{
    if (EXIT_SUCCESS != 0) return 1;
    if (EXIT_FAILURE != 1) return 2;
    if (RAND_MAX != 2147483647) return 3;

    /* rand() must stay within [0, RAND_MAX]. */
    srand(1u);
    for (int i = 0; i < 100; i++) {
        int r = rand();
        if (r < 0 || r > RAND_MAX) return 4;
    }
    return EXIT_SUCCESS;
}
