/* GCC Bug #65891 - -Wlogical-op now warns about logical ‘and’ of equal expressions even when different types/sizeofs are involved
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65891
 */


typedef int r_fun_t (int);

  r_fun_t * text_funcs[] = {0,0,0};

  int report (unsigned t)
  {
    typedef int s_fun_t (long, char);

    static s_fun_t * GUI_funcs[3];

    return (t < sizeof text_funcs / sizeof text_funcs[0] &&
            t < sizeof GUI_funcs / sizeof GUI_funcs[0]);
  }
// with
//   input: In function ‘report’:
//   input:8:58: warning: logical ‘and’ of equal expressions [-Wlogical-op]
     return (t < sizeof text_funcs / sizeof text_funcs[0] &&
//                                                           ^
// when these two conditions are about two different types, defined in two
// different locations, and the sizes are set differently.


