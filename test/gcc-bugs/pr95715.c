/* GCC Bug #95715 - __atomic_fetch_add accepts nonsense arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95715
 */


int i = 0;
// void* p = 0;
int r = __atomic_fetch_add(&i, &p, __ATOMIC_SEQ_CST);
int r = __atomic_fetch_add(&i, &p, __ATOMIC_SEQ_CST);
  int r = __atomic_fetch_add(&i, &p, __ATOMIC_SEQ_CST);


