/* GCC Bug #110238 - Incorrect "comparison between pointer and zero character constant" warning when comparing pointer to unsigned null pointer constant since r7-5677-ga9342885b149
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110238
 */
/* { dg-do compile } */
/* { dg-options "-Wpointer-compare" } */

/* Reporter's testcase: x == 0u is a null pointer constant comparison
 * and must not warn, but gcc warns "comparison between pointer and zero
 * character constant" because in C, char32_type_node is the same type
 * as unsigned int (comment 1).  Comparing to 0, 0l, 0ul does not warn. */
int main(void) {
    int *x = 0;
    return (x == 0u);
}