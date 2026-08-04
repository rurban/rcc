/* GCC Bug #111665 - internal compiler error: in c_objc_common_truthvalue_conversion with assume attribute and function decl (rather than a call)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111665
 */
/* { dg-do compile } */


extern int func(void *);
    int main(void)
    {
//         [[gnu::assume(func)]];
    }


