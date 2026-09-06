/* GCC Bug #94338 - struct member alignment is not carried over to alignment of struct variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94338
 */


#include <stdio.h>

struct {
 long a __attribute__((__aligned__(128)));
 long b __attribute__((__aligned__(128)));
} A __attribute__((__aligned__(4)));

struct {
 int a;
} B;

int main()
{
// 	printf("address of A %lx\n", &A);
// 	printf("address of A.a %lx\n", &A.a);
// 	printf("address of A.b %lx\n", &A.b);
// 	printf("address of B %lx\n", &B);
// 	printf("address of B.a %lx\n", &B.a);
 return 0;
}


