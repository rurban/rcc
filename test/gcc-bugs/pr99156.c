/* GCC Bug #99156 - __builtin_expect is folded too soon allowing an non-integer-constant-expr to become an integer-const-expr
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99156
 */


int maybe_vla(int n) {
//     goto label; /* { dg-error "cannot jump from" } */
int arr[({0;})];
    int arr[__builtin_expect(n-n, 0)];
 label:
    return sizeof(arr);
}

int main() {
    return maybe_vla(0);
}

// Basically __builtin_expect is folded too soon which allows the argument to be considered a constant integer expression :).


