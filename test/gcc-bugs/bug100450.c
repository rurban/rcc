/* GCC Bug #100450 - Missing ' ' space for '-E' preprocessing output, works with direct compilation
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100450
 */
/* { dg-do compile } */


#pragma omp forreduction(+:red)
  #pragma omp for reduction(+:red)

#define TEST(T) { \
     {T} \
}
#define CLAUSES reduction(+:red)
#define PARALLEL_FOR(X) TEST({ \
// _Pragma("omp for CLAUSES") \
})

void foo()
{
  int red = 0;
  int A[3] = {};
  #pragma omp parallel shared(red)
//   PARALLEL_FOR( for(int i=0; i < 3; i++) red += A[i]; )
}


