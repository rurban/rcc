/* GCC PR107166: a type-only (empty) declaration with an alignment
 * specifier and no declarator is legal C11 -- gcc/clang merely warn
 * "useless type name in empty declaration" (same as any other
 * declarator-less declaration), they don't reject it. rcc's parser hard
 * errored with "alignment specified for unnamed declaration" for *any*
 * has_alignas empty declaration, even outside the one case C11 6.7.5p2 via
 * GCC's `enum ... : type` extension actually disallows it (a fixed
 * underlying-type enum forward declaration, checked separately below and
 * still correctly rejected -- not exercised here since a rejected line
 * would break this file's own compile).
 *
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107166
 */
static int;
_Alignas(int) char;
long long;
_Alignas(16) struct { int x; };

int main(void)
{
    return 0;
}
