/* GCC Bug #95715 - __atomic_fetch_add accepts nonsense arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95715
 */
/* { dg-do compile } */


int i = 0;
void *p = 0;

void f(void)
{
  int r = __atomic_fetch_add(&i, &p, __ATOMIC_SEQ_CST);
  (void) r;
}

// GCC accepts this nonsense (adding a void* to an int) without any
// diagnostic, unlike clang and EDG which both reject it. Related variants
// also silently accepted (see comment #1): adding two void* together, or
// adding a double* to an int*.


