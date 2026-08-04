/* GCC Bug #111058 - __builtin_nans (and its friends for other floating-point types) compiles to an external call to __builtin_nans for unsupported tag
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111058
 */


// test.c
#include<stdio.h>

int main(void)
{
 _Float128 ret=__builtin_nansf128("NAN");
// 	printf("ret=%Lf\n",ret);

 return 0;	
}	
// Compile command: gcc test.c -s -o -
result:
x86_64:
//         ...
//      	movl	$.LC0, %edi
// 	call	__builtin_nansf128
//         ... 
LoongArch:
//        la.local	$r4,.LC0
// 	bl	%plt(__builtin_nansf128)


