/* GCC Bug #124699 - Expression using __builtin_ffsll() may not be considered constant
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124699
 */

int main() {
 sizeof(struct {
     _Static_assert(__builtin_ffsll(~0ULL) + 1 < 0);
 });
}


