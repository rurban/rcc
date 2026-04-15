#include <stdio.h>

// By separating this, optimize() CTFE cannot resolve `max` as a constant 
// inside main(), forcing the benchmark to measure actual native runtime.
int get_max() {
    return 200000;
}

int count_primes(int max) {
    int count = 0;
    for (int i = 2; i <= max; i = i + 1) {
        int is_prime = 1;
        for (int j = 2; j * j <= i; j = j + 1) {
            if (i % j == 0) {
                is_prime = 0;
                // No break statement in RCC yet, so it runs fully! 
                // This makes it a great heavy CPU benchmark.
            }
        }
        if (is_prime == 1) {
            count = count + 1;
        }
    }
    return count;
}

int main() {
    int max = get_max();
    printf("rcc benchmark start: primes up to %d\n", max);
    int ans = count_primes(max);
    printf("rcc benchmark end: %d primes found\n", ans);
    return 0;
}
