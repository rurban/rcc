/* GCC Bug #52923 - Warn if making external references to local stack memory
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52923
 */


{
  int a[100];
  struct test* t = (struct test*)malloc(sizeof(struct test));
//   // GIVE WARNING?
//   // "function returns with reference to local variable?"
//   t->ptr = a;
  return t;
}
// as you have no idea whether t is actually dereferenced in the caller.
// void* test_alloc_struct_on_stack_mem(void)
{
  struct test* t = (struct test*)alloca(sizeof(struct test));
//   t->ptr = NULL;
//   // GIVE WARNING?
//   // "function returns allocation from stack memory?"
  return t;
}

// for this I'd say yes, warn.  Similar for returning a pointer that was free()d.


