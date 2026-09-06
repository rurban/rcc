/* GCC Bug #98945 - gcc does not warn when assigning value of type int (*)() to variable of type int (*)(double)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98945
 */


int f()
{
        return 0;
}

int main()
{
        int (*p_float)(float f) = f;
        int (*p_double)(double d) = f;
        return 0;
}
// <stdin>:8:35: warning: initialization of 'int (*)(float)' from incompatible pointer type 'int (*)()' [-Wincompatible-pointer-types]


