/* GCC Bug #121129 - segmentation fault at process_init_element(unsigned long, c_expr, bool, obstack*)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121129
 */
/* { dg-do compile } */


// 13 |     [0 ... 5].O = {[1 ... 2].K[0 ... 1] = 4}, [5].O[2].K[2] = 5, 6, 7};
//    13 |     [0 ... 5].O = {[1 ... 2].K[0 ... 1] = 4}, [5].O[2].K[2] = 5, 6, 7};


