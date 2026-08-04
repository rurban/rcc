/* GCC Bug #33451 - Collapsing of offsetable memory operands.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=33451
 */


struct foo
{
   struct bar
   {
      int one[256];
      int two[256];
   };

   bar my_bar;
};

foo *fooptr;
// The members "one" and "two" are clearly offsetable memory operands, yet gcc does not accept them as operands to asm statements.
// asm( "..." : : "o" (fooptr->my_bar.two) ) // ERROR "memory operand not directly addressable".
// The expected behaviour is to generate code like this:
// mov fooptr, %eax
// mov 256(%eax), ...   // fooptr->my_bar.two


