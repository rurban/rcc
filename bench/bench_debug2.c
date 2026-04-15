#include <stdio.h>
static double arr[4][4];
int main(void) {
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            arr[i][j] = 1.0;
    printf("arr[0][0] = %.1f\n", arr[0][0]);
    return 0;
}
