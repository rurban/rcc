/* GCC Bug #124303 - C23: redeclaration of struct with function pointer with const pointer to struct as argument
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124303
 */
/* { dg-do compile } */


struct s { void (*p)(const struct s*); };
struct s { void (*p)(const struct s*); };
// <a href="https://godbolt.org/z/x7xcxEGTG">https://godbolt.org/z/x7xcxEGTG</a>


