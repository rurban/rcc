/* GCC Bug #88955 - transparent_union for vector types not accepted
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88955
 */


typedef unsigned long u64x2 __attribute__ ((vector_size (16)));

typedef union
{
        u64x2   u64;
} v128;

v128 bar(v128 x);
// v128 foo(v128 x)
{
//     x.u64 *= -1;
    return bar(x);
}
foo:
//         vpxor   %xmm1, %xmm1, %xmm1
//         vpsubq  %xmm0, %xmm1, %xmm0
//         jmp     bar


