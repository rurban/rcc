/* Test __builtin_isinf, __builtin_isinff, __builtin_isinfl */
#include <stdio.h>
#include <math.h>

int main() {
  float f = 1.01f * __FLT_MAX__;
  double d = 1.01 * __DBL_MAX__;
  long double ld = 1.01L * __LDBL_MAX__;

  if (__builtin_isinff(f) < 1) {
    printf("FAIL: __builtin_isinff(FLT_MAX*1.01f) = %d, expected >= 1\n",
           __builtin_isinff(f));
    return 1;
  }
  if (__builtin_isinf(d) < 1) {
    printf("FAIL: __builtin_isinf(DBL_MAX*1.01) = %d, expected >= 1\n",
           __builtin_isinf(d));
    return 2;
  }
  if (__builtin_isinfl(ld) < 1) {
    printf("FAIL: __builtin_isinfl(LDBL_MAX*1.01L) = %d, expected >= 1\n",
           __builtin_isinfl(ld));
    return 3;
  }
  /* Also check that finite values return 0 */
  if (__builtin_isinff(1.0f) != 0) {
    printf("FAIL: __builtin_isinff(1.0f) should be 0\n");
    return 4;
  }
  if (__builtin_isinf(1.0) != 0) {
    printf("FAIL: __builtin_isinf(1.0) should be 0\n");
    return 5;
  }

  printf("PASS\n");
  return 0;
}
