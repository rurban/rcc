/* GCC Bug #90680 - Misleading fixit warning with pointers to pointers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90680
 */


int main(void) {
    struct { int a; } **p;
//     p->a; // error
}
//     3 |     p->a;


