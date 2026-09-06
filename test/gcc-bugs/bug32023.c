/* GCC Bug #32023 - Message for lvalues could be improved if the cast was used as a lvalue
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=32023
 */


int main ()
{
int **a;
void **b;

// *b++;            /* works fine */
// *((void **)a)++; / gives error */

return 0;
}


