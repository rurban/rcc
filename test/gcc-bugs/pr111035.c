/* GCC Bug #111035 - Getting warning array subscript 0 is outside array bounds
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111035
 */


int main() {
    return *((int *)4092);
}
// Compiling with
// arm-none-eabi-gcc -save-temps=cwd -O2 -mcpu=cortex-m4 -Wall -c -o warn.o warn.c
// will give you
// warn.c: In function 'main':
// warn.c:7:12: warning: array subscript 0 is outside array bounds of 'int[0]' [-Warray-bounds=]
//     7 |     return *((int *)4092);
//       |            ^~~~~~~~~~~~~~
// cc1.exe: note: source object is likely at address zero
// If you change 4092 to 0 or any value 4096 or above, the warning will go away.
// In most microcontrollers, there is flash at address 0, so the code could make sense, it is not an academic problem.
// I think this should be improved!


