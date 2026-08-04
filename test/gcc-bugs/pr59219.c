/* GCC Bug #59219 - ____builtin___memcpy_chk and -fno-builtin-memcpy
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59219
 */
/* { dg-do compile } */


typedef __typeof__ (sizeof 0) size_t;

extern inline __attribute__ ((always_inline, artificial)) void*
memcpy (void* restrict d, const void* restrict s, size_t n) {
    return __builtin___memcpy_chk (d, s, n, __builtin_object_size (d, 1));
}

char b [4];

void foo (const void *p) {
    memcpy (b, p, sizeof b);
}
// 0000000000000010 <foo>:
//   10:	8b 07                	mov    (%rdi),%eax
//   12:	89 05 00 00 00 00    	mov    %eax,0(%rip)        # 18 <foo+0x8>
//   18:	c3                   	retq


