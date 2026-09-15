// Win64: a variadic function's own frame must reserve room for every
// nested call's register-argument staging slots (see gen_funcall()'s
// Win64 "always archive each register-passed arg into its own stack
// slot" comment) on top of its va_reg_save area, not instead of it.
//
// codegen()'s Pass-2 frame-size formula collapsed to `need =
// va_reg_save_ofs` for a variadic function -- a FIXED
// current_fn_stack_size-relative offset computed BEFORE the Pass-1 body
// walk that grows fn_struct_ret_total (struct-return temps, and every
// Win64 gen_funcall's per-call argument-staging slots). Once enough
// calls inside one variadic function staged enough register arguments
// to push fn_struct_ret_total past that fixed budget, the overflow
// slots landed BELOW the allocated frame -- at a stack address a
// subsequent nested call's own shadow-space/stack-argument writes then
// clobbered, corrupting the very values being staged for that call.
//
// Found via mingw-cross torture stdarg-4/va-arg-22: a variadic function
// with a switch of several cases, each independently calling another
// function with 4 register arguments (so each case's own staging reuses
// and grows the SAME function-wide fn_struct_ret_off counter), silently
// corrupted the 3rd+ case's arguments.
#include <stdarg.h>
#include <stdio.h>

static long sum4(int i, long a, long b, long c, long d) {
    return i + a + b + c + d;
}

// Each switch case independently stages 4 register arguments for its own
// sum4() call; fn_struct_ret_off keeps growing across cases (never reset
// between them), so by the 4th/5th case it has pushed well past the
// va_reg_save area's fixed 96-byte Win64 budget.
static long dispatch(int i, ...) {
    va_list ap;
    long a, b, c;
    long t;
    va_start(ap, i);
    switch (i) {
    case 0: t = sum4(i, 0, 0, 0, 0); break;
    case 1: a = va_arg(ap, long); t = sum4(i, a, 0, 0, 0); break;
    case 2: a = va_arg(ap, long); b = va_arg(ap, long); t = sum4(i, a, b, 0, 0); break;
    case 3:
        a = va_arg(ap, long);
        b = va_arg(ap, long);
        c = va_arg(ap, long);
        t = sum4(i, a, b, c, 0);
        break;
    default: t = -1; break;
    }
    va_end(ap);
    return t;
}

int main(void) {
    struct { long got, want; } cases[] = {
        {dispatch(0), 0},
        {dispatch(1, 18L), 19},
        {dispatch(2, 18L, 100L), 120},
        {dispatch(3, 18L, 100L, 300L), 421},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (cases[i].got != cases[i].want) {
            fprintf(stderr, "case %zu: got %ld, want %ld\n", i, cases[i].got, cases[i].want);
            return 1;
        }
    }
    return 0;
}
