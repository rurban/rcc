int main() {
    int x = 10;
    int y = 20;
    int *p = &x;
    *(p - 1) = 42; // y is located at p - 1
    if (y == 42) {
        return 0; // Success
    }
    return y;
}
