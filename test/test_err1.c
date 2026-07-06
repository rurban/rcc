/* Negative array size must be a compile error (C11 6.7.6.2p1).
 * Autoconf probes like AC_TYPE_INT32_T rely on this:
 *   static int test_array[1 - 2 * !(cond)];
 * must fail to compile when cond is false. */
static int test_array[1 - 2 * !(0)];

int main(void) { return 0; }
