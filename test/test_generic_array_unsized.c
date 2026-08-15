/* C11 6.2.7p1 array-type compatibility: an incomplete/unsized array type
 * `T[]` is compatible with any sized `T[N]` of the same element type, and
 * a VLA is compatible with a fixed array of the same element type
 * (length is not a compile-time compatibility criterion). type_equal()
 * (src/parser.c, backing both _Generic's association matching and
 * __builtin_types_compatible_p's redeclaration-compatibility use) used
 * to require an EXACT ->size match for TY_ARRAY and rejected TY_ARRAY
 * vs TY_VLA outright -- both real gcc-verified bugs found via noplate's
 * `array_lengthof()`/`vec2array()` macros, whose
 * `TYPE_CHECK(typeof(x[0])(*)[], &x)` idiom (assert "x really is some
 * array of T", regardless of length) and VLA-returning helpers always
 * missed every _Generic association as a result.
 */
#include <stdio.h>

int main(void) {
    /* Sized array pointer vs unsized-array association. */
    int arr[5] = {0};
    int (*p)[5] = &arr;
    int r1 = _Generic(p, int(*)[]: 1, default: 0);
    if (r1 != 1) {
        printf("FAIL: sized array pointer did not match unsized-array association\n");
        return 1;
    }

    /* VLA pointer (controlling expression, not an association type -- a
     * VLA type-name is not permitted as a _Generic association at all)
     * vs a fixed-size array association of the same element type. */
    int n = 5;
    int vla[n];
    for (int i = 0; i < n; i++) vla[i] = i;
    int (*vp)[n] = &vla;
    int r2 = _Generic(vp, int(*)[5]: 2, default: 0);
    if (r2 != 2) {
        printf("FAIL: VLA pointer did not match fixed-size array association\n");
        return 2;
    }

    printf("OK\n");
    return 0;
}
