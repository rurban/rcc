/* GCC Bug #71597 - Confusing error for incompatible enums, wrong previous declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71597
 */


enum { a } x; // (1)
unsigned x; // (2)
enum { b } x; // (3)

int main()
{
}
 enum { b } x; // (3)
 unsigned x; // (2)
// It would be better to display the conflicting declaration (1) instead of just the previous one (2).


