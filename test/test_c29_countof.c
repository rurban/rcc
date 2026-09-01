/* C29 (WG14 N3369/N3469): the `_Countof` operator yields the number of
 * elements of an array operand -- unlike the old sizeof(x)/sizeof(x[0])
 * trick, applying it to a non-array is a hard compile error rather than
 * silently computing garbage from a decayed pointer. `<stdcountof.h>`
 * exposes it as the `countof()` macro.
 */
#include "test_common.h"
#include <stdcountof.h>

int g_fixed[10];
int g_matrix[3][7]; /* _Countof must report the OUTER dimension only */

static int vla_countof(int n)
{
    int vla[n];
    return (int)_Countof(vla);
}

static int vla_2d_countof(int n)
{
    int vla[n][5]; /* outer dimension variable, inner fixed */
    return (int)_Countof(vla);
}

int main(void)
{
    if (_Countof(g_fixed) != 10) {
        printf("FAIL: _Countof(g_fixed) != 10 (got %zu)\n", _Countof(g_fixed));
        return 1;
    }
    if (_Countof(g_matrix) != 3) {
        printf("FAIL: _Countof(g_matrix) != 3 (got %zu)\n", _Countof(g_matrix));
        return 2;
    }

    /* Type-name form. */
    if (_Countof(int[5]) != 5) {
        printf("FAIL: _Countof(int[5]) != 5 (got %zu)\n", _Countof(int[5]));
        return 3;
    }

    /* A local, stack fixed-size array. */
    char local[42];
    if (_Countof(local) != 42) {
        printf("FAIL: _Countof(local) != 42 (got %zu)\n", _Countof(local));
        return 4;
    }

    /* countof() macro from <stdcountof.h> must expand to the same
     * operator. */
    if (countof(g_fixed) != 10) {
        printf("FAIL: countof(g_fixed) != 10 (got %zu)\n", countof(g_fixed));
        return 5;
    }

    /* VLA: runtime-evaluated count, both 1-D and the outer dimension of
     * a 2-D VLA. */
    if (vla_countof(7) != 7) {
        printf("FAIL: vla_countof(7) != 7 (got %d)\n", vla_countof(7));
        return 6;
    }
    if (vla_countof(0) != 0) {
        printf("FAIL: vla_countof(0) != 0 (got %d)\n", vla_countof(0));
        return 7;
    }
    if (vla_2d_countof(4) != 4) {
        printf("FAIL: vla_2d_countof(4) != 4 (got %d)\n", vla_2d_countof(4));
        return 8;
    }

    /* A compile-time-constant _Countof must fold into a genuine integer
     * constant expression (usable as an array bound / static_assert). */
    static_assert(_Countof(g_fixed) == 10, "constant _Countof must fold");
    int arr2[_Countof(g_fixed)];
    if ((int)(sizeof(arr2) / sizeof(int)) != 10) {
        printf("FAIL: _Countof(g_fixed) not usable as an array bound\n");
        return 9;
    }

    /* Non-array operand must be a hard compile error (checked via a
     * subprocess, since this translation unit itself must compile). */
    {
        const char *rcc = find_rcc();
        const char *td = get_tmpdir();
        int pid = (int)getpid();
        char srcf[256], objf[256], cmd[768];
        snprintf(srcf, sizeof(srcf), "%s/test_c29_countof_err_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_c29_countof_err_%d.o", td, pid);

        static const char src[] =
            "int main(void) { int x = 1; return (int)_Countof(x); }\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 10; }
        fputs(src, f);
        fclose(f);

        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int wrc = system(cmd);
        remove(objf);
        remove(srcf);
        if (wrc == 0) {
            printf("FAIL: '_Countof' on a non-array must be a compile error\n");
            return 11;
        }
    }

    return 0;
}
