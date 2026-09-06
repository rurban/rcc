/* GCC Bug #108995 - Missed signed integer overflow checks in UBsan? since r8-343-g2bf54d93f159210d
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108995
 */


int printf(const char *, ...);
int a;
const int b = 44514;
int *c = &a;
void main(void) {
//   *c = 65526 * b / 6;
//   printf("%d\n", a);
}
// Ubsan did not emit any message. However, the outputs are different.
// -229690488
// 486137394


