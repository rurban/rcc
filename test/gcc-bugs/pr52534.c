/* GCC Bug #52534 - gcc doesn't detect incorrect expression in call to va_start
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52534
 */


int maxof(int, ...) ;
void f(void);

int maxof(int n_args, ...){
    register int i;
    int max, a;
    va_list ap;
//     va_start(ap, (unsigned int)n_args);
    max = va_arg(ap, int);
    for(i = 2; i <= n_args; i++) {
       if((a = va_arg(ap, int)) > max) max = a;
    }
//     va_end(ap);
    return max;
}
// Intel compiler finds the bug:
// vaarg.c(12): error: incorrect use of va_start
//       va_start(ap, (unsigned int)n_args);
//       ^


