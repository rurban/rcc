/* GCC Bug #108483 - gcc warns about suspicious constructs for unevaluted ?: operand
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108483
 */


typedef __SIZE_TYPE__ size_t;
typedef __UINTPTR_TYPE__ uintptr_t;
#define NULL ((void *) 0)
#define ARRAY_SIZE_MAYBENULL(x) _Generic((x), void*: (size_t) (uintptr_t) ((char (*) [__builtin_constant_p (x) && (x) == 0 ? 1 : _Generic((x), void*: -1, default: 1)]) 0), default: \
  sizeof(x)/sizeof(_Generic((x), void*: (x), default:*(x))))

int a[10];
int *q;
void *r;
int p[] = {
// ARRAY_SIZE_MAYBENULL (a),
// ARRAY_SIZE_MAYBENULL (NULL),
// ARRAY_SIZE_MAYBENULL ((const void *) 0),
// ARRAY_SIZE_MAYBENULL ((volatile void *) 0),
// ARRAY_SIZE_MAYBENULL ((void *restrict) 0),
// ARRAY_SIZE_MAYBENULL ((const volatile void *) 0),
// ARRAY_SIZE_MAYBENULL (q),
// ARRAY_SIZE_MAYBENULL (r),
// ARRAY_SIZE_MAYBENULL ((void *) (uintptr_t) 0x1234abc)
};
// This warns -Wsizeof-pointer-div on the const void *, volatile void *, const volatile void * and q cases and errors on the last 2.
// clang seems to also emit those 4 -Wsizeof-pointer-div warnings, but doesn't emit any errors nor warnings on the last 2.


