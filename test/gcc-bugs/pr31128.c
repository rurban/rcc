/* GCC Bug #31128 - __builtin_stack_restore/__builtin_stack_save should not be exposed to the user
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31128
 */


int f(int t1)
{
  {
     int t = 0;
     int a[t];
     int *b = __builtin_alloca(t1);
  }
}


