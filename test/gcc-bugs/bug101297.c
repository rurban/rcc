/* GCC Bug #101297 - Spurious comma accepted at the end of #pragma omp atomic
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101297
 */
/* { dg-do compile } */


int i;
// void
// foo (void)
{
  #pragma omp atomic update,	/* { dg-error "expected end of line before ',' token" } */
//   i++;
  #pragma omp atomic update,,	/* { dg-error "expected end of line before ',' token" } */
//   i++;
}

// diagnoses with -fopenmp only the second error and not the first one (seems both clang and ICC suffer from the same bug though).


