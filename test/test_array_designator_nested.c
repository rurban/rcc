/* A designated initializer for a multi-dimensional array can chain two (or
 * more) "[N]" designators without repeating the outer array's brace level,
 * e.g. "[i][j] = val" — standard C99 designated-initializer syntax for
 * skipping straight to an element of a nested array. rcc's designated-array
 * initializer loop (both the file-scope global_init_one() and the local
 * local_init_one() paths in src/parser.c) handled the first "[N][M]=val"
 * entry correctly, but after applying it, jumped straight back to the top
 * of the loop with a bare "continue" instead of consuming the trailing
 * comma the way every other element in the loop does. The next iteration
 * then saw the comma sitting at the front of the token stream and tried to
 * parse *it* as the start of a new initializer expression, failing with
 * "expected an expression". Any array with more than one "[N][M]=val"
 * designator (or a single one followed by more elements) hit this — a
 * *single* nested designator that happened to be the very last element
 * before the closing brace could accidentally look fine only because
 * skip_initializer-style leftover parsing wasn't reached, but the general
 * case broke.  The global path additionally passed the wrong type
 * (ty->base, the *inner array* type, instead of ty->base->base, the
 * scalar element type) down to the recursive initializer call.
 *
 * Found via a real Linux kernel build: kernel/cpu.c's cpu_bit_bitmap[]
 * definition builds its designated initializers through nested
 * MASK_DECLARE_1/2/4/8(x) macros that bottom out in "[x+1][0] = (1UL <<
 * (x))" entries, many of which appear back-to-back separated by commas in
 * a single brace-enclosed initializer list.
 */
int global_arr[3][3] = {
    [0][0] = 10,
    [1][1] = 20,
    [2][2] = 30,
};

int main(void)
{
    if (global_arr[0][0] != 10) return 1;
    if (global_arr[1][1] != 20) return 2;
    if (global_arr[2][2] != 30) return 3;
    if (global_arr[0][1] != 0) return 4;

    int local_arr[3][3] = {
        [0][0] = 1,
        [1][1] = 2,
        [2][2] = 3,
    };
    if (local_arr[0][0] != 1) return 5;
    if (local_arr[1][1] != 2) return 6;
    if (local_arr[2][2] != 3) return 7;
    if (local_arr[1][0] != 0) return 8;

    return 0;
}
