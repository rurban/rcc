/* GCC Bug #92220 - -Wconversion generates a false warning for modulo expression when the modulus has smaller type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92220
 */


#include <stdint.h>

int main ( void )
{
  volatile uint64_t a = 123;
  volatile uint16_t b = 2;

{aka 'short unsigned int'} may change value [-Wconversion]

  uint16_t result = a % b;

  return result;
}
// TypeB ModuloToSmallerSize ( const TypeA a, const TypeB b ) throw()
{
  if constexpr ( sizeof( TypeA ) <= sizeof( TypeB ) )
  {
    return a % b;
  }
  {
    return static_cast< TypeB >( a % b );
  }
}


