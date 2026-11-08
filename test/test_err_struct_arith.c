/* A non-vector struct/union is not a valid operand of binary arithmetic
 * (GCC: "invalid operands to binary +"). rcc accepted it, so Ruby's
 * configure probe `mcontext_t test(mcontext_t mc) {return mc+1;}` wrongly
 * reported mcontext_t as a pointer type on glibc.
 */
typedef struct { int a[4]; } S;
S test(S mc) { return mc + 1; }
int main(void) {
    S s = {{0}};
    test(s);
    return 0;
}
