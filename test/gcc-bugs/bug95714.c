/* GCC Bug #95714 - Poor locations for errors in calls to __atomic built-ins
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95714
 */


int main()
{
  char c;
  int i;
  __atomic_load(&c, &i, __ATOMIC_SEQ_CST);
}
//     5 |   __atomic_load(&c, &i, __ATOMIC_SEQ_CST);
// ab.cc: In function 'int main()':
//     5 |   __atomic_load(&c, &i, __ATOMIC_SEQ_CST);
//     5 |   __atomic_load(&c, &i, __ATOMIC_SEQ_CST);
//     5 |   __atomic_load(&c, &i, __ATOMIC_SEQ_CST);


