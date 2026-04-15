#include <stdio.h>

#define MSZ 128
static double A[MSZ][MSZ], B[MSZ][MSZ], C[MSZ][MSZ];

void matmul(void) {
    int i, j, k;
    for (i = 0; i < MSZ; i++)
        for (j = 0; j < MSZ; j++) {
            double sum = 0.0;
            for (k = 0; k < MSZ; k++)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
}

int main(void) {
    int i, j;
    printf("matmul init...\n");
    for (i = 0; i < MSZ; i++)
        for (j = 0; j < MSZ; j++) {
            A[i][j] = (double)(i + j) * 0.01;
            B[i][j] = (double)(i - j) * 0.01;
        }
    printf("matmul run...\n");
    matmul();
    printf("C[64][64] = %.6f\n", C[64][64]);
    return 0;
}
