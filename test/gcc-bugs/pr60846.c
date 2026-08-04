/* GCC Bug #60846 - Add 128-bit integer types for general use on 32-bit/64-bit CPUs
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60846
 */


// Note that passing 128-bit types or larger is already supported on 32-bit arch: a struct or a union is such a type, and there's also _Decimal128 for the 128-bit size. So, isn't the ABI already defined? (In addition to the type size, there are endianness issues, but I suspect that nowadays, the intent would be to avoid mixed endianness.)


