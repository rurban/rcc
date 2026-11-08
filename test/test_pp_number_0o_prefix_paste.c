/* `0op` in a macro body is one pp-number. rcc's C2Y `0o` octal-prefix lexing
 * split it into `0o` + `p`, so metalang99's `ML99_PRIV_EVAL_ ## 0op` pasted
 * to `ML99_PRIV_EVAL_0o p` and the evaluator's object-like
 * ML99_PRIV_EVAL_0op never expanded (every datatype99 use failed).
 */
#include <stdio.h>

#define EVAL_0op 77
#define EVAL_0v 55
#define P(a, b) a##b
#define P2(a, b) P(a, b)
#define KIND(k, ...) k
#define TAG 0op
#define A P(EVAL_, 0op)
#define B P2(EVAL_, KIND(0op, 1))
#define C P2(EVAL_, TAG)
#define D P(EVAL_, 0v)

int main(void) {
    int a = A, b = B, c = C, d = D;
    if (a != 77 || b != 77 || c != 77 || d != 55) {
        printf("FAIL: %d %d %d %d\n", a, b, c, d);
        return 1;
    }
    printf("OK\n");
    return 0;
}
