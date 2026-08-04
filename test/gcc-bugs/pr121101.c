/* GCC Bug #121101 - __builtin_assoc_barrier vs function types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121101
 */


typedef void (*ftype)(void );
void f(void);
ftype get_free(void) { return __builtin_assoc_barrier(f); }


