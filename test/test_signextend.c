#include <stdio.h>

char buf[256];
char *pp;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    pp = buf + 4;
    if (pp == buf
        // used to crash here
        || (pp[-1] == 0)) {
        printf("OK\n");
    }
    return 0;
}
