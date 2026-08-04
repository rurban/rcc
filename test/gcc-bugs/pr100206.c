/* GCC Bug #100206 - aarch64: UB in varasm.c:output_object_block and assembly failure
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100206 */


const char a;
const char b[] = {
//   'a',
//   [1234] = 'b',
  [(0x7fffffffffffffffL - 1)] = '\0'   /* { dg-warning "conversion" } */
};


