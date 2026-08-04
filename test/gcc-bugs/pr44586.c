/* GCC Bug #44586 - gcc does not warn about casting non-variadic types to variadic types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44586
 */
/* { dg-do compile } */


int foo(int x, int y) {
        return x+y;
}

void f(int x, int y) {
       bar_t bar;

//        /* Cast foo to variadic type... undefined behaviour */
       bar = (bar_t) foo;
//        (*bar)(x,y);
}


