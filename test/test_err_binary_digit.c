/* C23 binary-integer-constant: the digit sequence after "0b" may contain
 * only 0/1. A stray decimal digit immediately following must be a
 * compile error, not silently stop consuming the token. */
int x = 0b012;

int main(void) { return 0; }
