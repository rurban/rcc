/* GCC Bug #88727 - Diagnostics improvement: Detection of undefined behaviour. Incomplete type in tenative definition with internal linkage. [-Wtentative-definition-incomplete-type]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88727
 */
/* { dg-do compile } */
/* { dg-options "-Wall -Wextra -std=c11 -pedantic-errors" } */

static struct S s;

int main()
{
}

struct S { int x; };

