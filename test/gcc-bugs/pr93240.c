/* GCC Bug #93240 - [frontend] 'align_value' attribute not suported
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93240
 */


typedef double * aligned_double_ptr __attribute__((align_value(64)));

struct S {
    aligned_double_ptr y;
};

void foo(S*s){
    s->y[0] = 0;
}
//     1 | typedef double * aligned_double_ptr __attribute__((align_value(64)));


