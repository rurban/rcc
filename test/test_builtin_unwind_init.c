// __builtin_unwind_init(): must link (gcc builtin, spills the callee-saved registers into the frame)
// and keep values across the call.
#include <stdio.h>

static int id(int x) { return x; }

int main(void) {
    int a = id(1), b = id(2), c = id(3), d = id(4), e = id(5);
    __builtin_unwind_init();
    if (a + b + c + d + e != 15) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
