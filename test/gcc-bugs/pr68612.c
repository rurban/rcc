/* GCC Bug #68612 - Const-compatibility in C
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68612
 */


void func (int const * const * foo)
{
    if (foo)
        ;
}

int main ()
{
    int * * foo = 0;
    func (foo);
}
// ```
// fails with
//     warning: passing argument 1 of 'func' from incompatible pointer type [-Wincompatible-pointer-types]
//     note: expected 'const int **' but argument is of type 'int **'
// as per the C99 specification. C++ is more liberal in this respect, but no less safe, since the above will compile without warnings. (Note that if the parameter type for `func` in the above example is instead `int const * *`, there exist slightly obscure ways for code to modify `**foo` without warnings or errors being generated, thus violating const-correctness. This is not the case as long as the parameter type is `int const * const *`.)

// Would it possible to enable a feature on the C compiler and GCC driver for const-compatibility to work as it does for C++, in the more liberal manner? As a point of annoyance for many C programmers (in my experience), this would at least allow those of us who don't care about standards compliance so much to circumvent this design deficiency.


