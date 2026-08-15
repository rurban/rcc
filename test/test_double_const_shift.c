/* A `double`-typed static initializer whose value is a purely-integer
 * constant expression (shift, bitwise op) that eval_double_const_expr()
 * doesn't itself fold -- e.g. njs's "NJS_MAX_SAFE_INTEGER" idiom,
 * `(double)((1LL << 53) - 1)`. C's usual conversions implicitly convert
 * any integer constant to the target floating type; rcc's
 * eval_double_const_expr() had no fallback to the integer evaluator for
 * node kinds it doesn't itself handle (ND_SHL et al.), so this hard
 * "expected constant expression in initializer"-errored instead of
 * folding.
 */
#include <stdio.h>

double max_safe_integer = ((1LL << 53) - 1);
double bitand_val = (0xFF & 0x0F);
struct s {
    double d;
} nested = {.d = ((1LL << 4) - 1)};

int main(void) {
    if (max_safe_integer != 9007199254740991.0) {
        printf("FAIL: shift-based double constant wrong: %f\n", max_safe_integer);
        return 1;
    }
    if (bitand_val != 15.0) {
        printf("FAIL: bitand-based double constant wrong: %f\n", bitand_val);
        return 2;
    }
    if (nested.d != 15.0) {
        printf("FAIL: nested struct shift-based double constant wrong: %f\n", nested.d);
        return 3;
    }
    printf("OK\n");
    return 0;
}
