#include <stdio.h>

// ===== 1. Fibonacci (recursive, integer heavy) =====
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

// ===== 2. Ackermann (deep recursion stress) =====
int ackermann(int m, int n) {
    if (m == 0) return n + 1;
    if (n == 0) return ackermann(m - 1, 1);
    return ackermann(m - 1, ackermann(m, n - 1));
}

// ===== 3. Sieve of Eratosthenes (array + branch) =====
int sieve(int limit) {
    static char is_prime[1000001];
    int i, j, count = 0;
    for (i = 0; i <= limit; i++) is_prime[i] = 1;
    is_prime[0] = is_prime[1] = 0;
    for (i = 2; i * i <= limit; i++) {
        if (is_prime[i]) {
            for (j = i * i; j <= limit; j += i)
                is_prime[j] = 0;
        }
    }
    for (i = 2; i <= limit; i++)
        if (is_prime[i]) count++;
    return count;
}

// ===== 4. Matrix multiply (loop + memory access) =====
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

// ===== 5. Float math (trig / sqrt approximation loop) =====
double float_bench(int iters) {
    double sum = 0.0;
    int i;
    for (i = 1; i <= iters; i++) {
        double x = (double)i * 0.001;
        // Taylor-ish: x - x^3/6 + x^5/120
        double x2 = x * x;
        double x3 = x2 * x;
        double x5 = x3 * x2;
        sum += x - x3 / 6.0 + x5 / 120.0;
    }
    return sum;
}

// ===== 6. Bubble sort (swap heavy) =====
void bubble_sort(int *arr, int n) {
    int i, j, tmp;
    for (i = 0; i < n - 1; i++)
        for (j = 0; j < n - 1 - i; j++)
            if (arr[j] > arr[j + 1]) {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
}

static int sort_arr[5000];

int main(void) {
    int i, j;

    // 1. Fibonacci
    int fib_result = fib(38);
    printf("fib(38)          = %d\n", fib_result);

    // 2. Ackermann
    int ack_result = ackermann(3, 10);
    printf("ackermann(3,10)  = %d\n", ack_result);

    // 3. Sieve
    int primes = sieve(1000000);
    printf("primes <= 1M     = %d\n", primes);

    // 4. Matrix multiply
    for (i = 0; i < MSZ; i++)
        for (j = 0; j < MSZ; j++) {
            A[i][j] = (double)(i + j) * 0.01;
            B[i][j] = (double)(i - j) * 0.01;
        }
    matmul();
    printf("C[64][64]        = %.6f\n", C[64][64]);

    // 5. Float math
    double fsum = float_bench(500000);
    printf("float_bench(500k)= %.6f\n", fsum);

    // 6. Bubble sort
    for (i = 0; i < 5000; i++)
        sort_arr[i] = 5000 - i;
    bubble_sort(sort_arr, 5000);
    printf("sort[0]=%d sort[4999]=%d\n", sort_arr[0], sort_arr[4999]);

    printf("ALL BENCHMARKS DONE\n");
    return 0;
}
