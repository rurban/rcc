/* GCC Bug #108310 - Some warnings that -Wtraditional-conversion causes to be emitted aren't actually controlled by it
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108310
 */


#include <architecture/byte_order.h>
#include <sys/types.h>
#include <inttypes.h>

// uint32_t foo(float x)
{
 return OSSwapHostToLittleInt32(x);
}

// int32_t bar(double x)
{
 return OSSwapHostToBigInt32(x);
}
// $
// Unfortunately, the only warning that it produces so far *is* actually controlled by -Wtraditional-conversion properly, so it's not an example of this bug (yet):
// In file included from /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/libkern/OSByteOrder.h:33,
//                  from /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/architecture/byte_order.h:38,
//                  from Wtraditional_conversion_darwin.c:1:
// Wtraditional_conversion_darwin.c: In function 'bar':
// Wtraditional_conversion_darwin.c:12:37: warning: passing argument 1 of '_OSSwapInt32' as integer rather than floating due to prototype [-Wtraditional-conversion]
//    12 |         return OSSwapHostToBigInt32(x);
//       |                                     ^
// $
// ref: <a href="https://github.com/cooljeanius/gcc_bugs/blob/master/Wtraditional_conversion_darwin.c">https://github.com/cooljeanius/gcc_bugs/blob/master/Wtraditional_conversion_darwin.c</a>


