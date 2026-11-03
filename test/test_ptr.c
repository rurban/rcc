// Pointer arithmetic scales by the element size: `p - 1` addresses the
// previous int. Uses an array so the layout is defined (the test used to
// assume two unrelated locals sit next to each other on the stack, which an
// optimizing compiler is free to break).
int main() {
    int a[2] = {20, 10};
    int *p = &a[1];
    *(p - 1) = 42;
    if (a[0] == 42) {
        return 0; // Success
    }
    return a[0];
}
