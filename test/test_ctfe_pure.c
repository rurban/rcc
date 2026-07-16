// Test that CTFE folds pure/const functions at -O1.
// Verification: compile with -O1, link and run.
//   objdump -d | grep -c 'call.*strlen' should be 0.
//   objdump -d | grep -c 'call.*abs'    should be 0.
// Compile with -O0: runtime calls present.

int printf(const char *fmt, ...);
int strlen(const char *s);
int abs(int n);

__attribute__((const)) static int add3(const int x) { return x + 3; }
__attribute__((pure)) static int get42(void) { return 42; }
[[unsequenced]] static int mul2(const int x) { return x * 2; }
[[reproducible]] static int get7(void) { return 7; }

int main() {
    if (strlen("hello") != 5)  return 1;
    if (abs(-42) != 42)        return 2;
    if (add3(10) != 13)        return 3;
    if (get42() != 42)         return 4;
    if (mul2(11) != 22)        return 5;
    if (get7() != 7)           return 6;

    printf("CTFE pure folding: PASS\n");
    return 0;
}
