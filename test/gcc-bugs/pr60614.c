/* GCC Bug #60614 - -Wtype-limits fails to warn on unsigned bitfields
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60614
 */


#include <stdio.h>

struct {
 unsigned char field1 :3;
 unsigned char field2 :5;
} teststruct;

int main ( void ) {

 unsigned char test;

	if (teststruct.field1 < 0) //issues no warning
// 		printf("Field1 was negative\n");

	if (test < 0) //issues warning
// 		printf("Test was negative\n");

 return 0;
}
  if (test < 0)


