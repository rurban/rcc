// C11 conformance test — derived from autoconf C11 feature check
// Tests: _Alignas, _Alignof, _Static_assert, _Noreturn, u8 literals,
// anonymous structs/unions, duplicate typedefs

// Verify C11 __STDC_VERSION__
#if __STDC_VERSION__ < 201112L
#error "Compiler does not advertise C11 conformance"
#endif

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

// _Alignas
char _Alignas(double) aligned_as_double;
char _Alignas(0) no_special_alignment;
extern char aligned_as_int;
char _Alignas(0) _Alignas(int) aligned_as_int;

// _Alignof
enum {
    int_alignment = _Alignof(int),
    int_array_alignment = _Alignof(int[100]),
    char_alignment = _Alignof(char)
};
_Static_assert(0 < -_Alignof(int), "_Alignof is signed");

// _Noreturn
int _Noreturn does_not_return(void) { for (;;) continue; }

// _Static_assert inside struct
struct test_static_assert {
    int x;
    _Static_assert(sizeof(int) <= sizeof(long int),
                   "_Static_assert does not work in struct");
    long int y;
};

// Duplicate typedefs (C11 allows)
typedef long *long_ptr;
typedef long int *long_ptr;
typedef long_ptr long_ptr;

// Anonymous structures and unions (C11 6.7.2.1 Example 1)
struct anonymous {
    union {
        struct { int i; int j; };
        struct { int k; long int l; } w;
    };
    int m;
} v1;

int main(void)
{
    // Verify anonymous struct layout: i and w.k share offset
    _Static_assert((__builtin_offsetof(struct anonymous, i)
                    == __builtin_offsetof(struct anonymous, w.k)),
                   "Anonymous union alignment botch");
    v1.i = 2;
    v1.w.k = 5;
    if (v1.i != 5) return 1;

    // Verify _Alignas and _Alignof
    if (_Alignof(char) != 1) return 2;
    if (_Alignof(int) < 4) return 3;

    return 0;
}
