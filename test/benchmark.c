int printf(char *fmt, ...);

int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int start = 35;
    printf("rcc benchmark start: fib(35)\n");
    int ans = fib(35);
    printf("rcc benchmark end: fib(35) = %d\n", ans);
    return 0;
}
