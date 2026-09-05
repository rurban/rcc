/* GCC Bug #84163 - [avr] Allow address space qualifier for compound literals
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84163
 */
/* { dg-do compile } */


#define FSTR(X) ((const __flash char[]) { X } )

const __flash char *pet = FSTR ("pet");
// 	.section	.progmem.data,"a",@progbits
// 	.type	__compound_literal.0, @object
// 	.size	__compound_literal.0, 4
// __compound_literal.0:
// 	.string	"pet"
// .global	pet
// 	.data
// 	.type	pet, @object
// 	.size	pet, 2
// pet:
// 	.word	__compound_literal.0
// Hence this works great and perfectly as expected.  Unfortunately, this does not work locally in a function:
const __flash char* get_pet (void)
{
  static const __flash char *pet = FSTR ("pet");
  return pet;
}
// literal.c: In function 'get_pet':
// literal.c:7:3: error: compound literal qualified by address-space qualifier
//   static const __flash char *pet = FSTR ("pet");
//    ^~~~~~
// Would be great if the C front end also supported such compound literals with the obvious semantics as a GNU-C extension (which __flash and other address-spaces already are).


