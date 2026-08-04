/* GCC Bug #96810 - This is a case that gcc shoud not compile successfully, but gcc acts opposite.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96810
 */
/* { dg-do compile } */


int A[30];

// foo (void)
{
  #pragma omp target data map (A[0:4])
  #pragma omp target
  A[2] = 0;
}

// main ()
{
  #pragma omp target data map (A)
  foo ();
  return 0;
}
void bar (int *, int);

// foo (void)
{
  int A[30];
  bar (A, 0);
  #pragma omp target data map (A[0:4])
  #pragma omp target
  A[2] = 0;
  bar (A, 1);
}
// Both are completely valid and e.g. the latter could work just fine at runtime if bar performs #pragma omp target enter data (arg1[:30]) for arg2 0 and exit data for arg2 non-zero.
// Perhaps a warning might be ok, but it still can have many false positives (unless the compiler can prove that the array can't be mapped before the outer target data).


