#include <stdio.h>
struct stringpool_t {
    char stringpool_str0[4];
};
int main(void) {
    int x = (int)(size_t) & ((struct stringpool_t *)0)->stringpool_str0;
    printf("x = %d\n", x);
    return 0;
}
