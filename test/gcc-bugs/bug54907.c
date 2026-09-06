/* GCC Bug #54907 - post increasing a value pointed by p in subexpression of an expression modifying p saves the increased value in the wrong place
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54907
 */


#include <stdio.h>

int main() {
    char s[] = "axxxxx";
    char *p = s;

//     printf("s = %s in the beginning.\n"
//            "p is pointed at the %d-th char.\n", s, p - s);
//     //p = p + (*p)++ * 3 + 2 - 'a' * 3; // (1)
    p += (*p)++ * 3 + 2 - 'a' * 3; // (2)
//     printf("p is moved ahead by %d steps\n", p - s);
//     printf("s = %s after the operation.\n", s);
    return 0;
}
// The expected result is "bxxxxx". But the output is "axbxxx".
// Maybe in the wrong code, when it saves the value, it lookups the address again by *p, but p is modified in the expression.
// As discussed in stackoverflow, <a href="http://stackoverflow.com/questions/12823663/would-p-p-p-3-c-cause-an-undefined-behavior?answertab=votes#tab-top">http://stackoverflow.com/questions/12823663/would-p-p-p-3-c-cause-an-undefined-behavior?answertab=votes#tab-top</a> most people think it's a bug of gcc.
// Bug found in gcc 4.4.6, 4.7.1, g++ 4.4.6. g++ 4.7.1 produces the correct result.


