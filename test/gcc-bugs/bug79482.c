/* GCC Bug #79482 - _Static_assert(__builtin_constant_p(x)):
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79482
 */
/* { dg-do compile } */


int main(int argc, char *argv[]) { 
    int x = 0;

    _Static_assert(__builtin_constant_p(x) ? 1 : 2, "error");

    return 0;
}

