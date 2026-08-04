/* GCC Bug #121282 - Complex float static initialization vs -frounding-math
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121282
 */


#include <complex.h>

#define FLT	float
#define DBL	double
#define LDBL	long double

#define FLT_CMPLX FLT _Complex
#define DBL_CMPLX DBL _Complex
#define LDBL_CMPLX LDBL _Complex

#define FP_F(v) v##f
#define FP_D(v) v
#define FP_LD(v) v##L

static const  FLT_CMPLX  flt_flt  =  FP_F(.6666666666666666666666666666666666666); /* 1 */
static const  FLT_CMPLX  flt_dbl  =  FP_D(.6666666666666666666666666666666666666); /* 2 */
static const  FLT_CMPLX  flt_ldbl = FP_LD(.6666666666666666666666666666666666666); /* 3 */

static const  DBL_CMPLX  dbl_flt  =  FP_F(.6666666666666666666666666666666666666); /* 4 */
static const  DBL_CMPLX  dbl_dbl  =  FP_D(.6666666666666666666666666666666666666); /* 5 */
static const  DBL_CMPLX  dbl_ldbl = FP_LD(.6666666666666666666666666666666666666); /* 6 */

static const LDBL_CMPLX ldbl_flt  =  FP_F(.6666666666666666666666666666666666666); /* 7 */
static const LDBL_CMPLX ldbl_dbl  =  FP_D(.6666666666666666666666666666666666666); /* 8 */
static const LDBL_CMPLX ldbl_ldbl = FP_LD(.6666666666666666666666666666666666666); /* 9 */

int main(void){
  return 0;
}
// gets these error messages:
//  error: initializer element is not computable at load time
// These are the flags I pass to gcc:
// export CFLAGS="-H -std=gnu23 -O0 -march=native -mhard-float -mfpmath=387 -mieee-fp \
//  -enable-decimal-float=yes \
//  -fexcess-precision=standard \
//  -ffloat-store \
//  -ffp-contract=off \
//  -fmath-errno \
//  -fno-associative-math \
//  -fno-builtin \
//  -fno-cx-limited-range \
//  -fno-fast-math \
//  -fno-finite-math-only \
//  -fno-reciprocal-math \
//  -fno-unsafe-math-optimizations \
//  -frounding-math \
//  -fsignaling-nans \
//  -fsigned-zeros \
//  -ftrapping-math \
 ${INCS}"


