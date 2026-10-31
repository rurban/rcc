// -fstack-protector-strong: canary must fire on stack-buffer-overflow
int vulnerable(void) {
    int buf[4];
    volatile int *p = buf;
    for (int i = 0; i < 20; i++)
        p[i] = i; // overflow: writes 20 ints into 4-int buffer
    return buf[0];
}

int safe(void) {
    int x = 42;
    return x;
}

int main(void) {
    if (safe() != 42) return 1;
    volatile int r = vulnerable();
    (void)r;
    return 0;
}
