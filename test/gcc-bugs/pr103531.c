/* GCC Bug #103531 - Propose compiler warning when ceil/ceilf used on integral value
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103531
 */


int covers_half = ceil(x / 2); 

// If x is a floating-point value, this codes will act as expected; but if it's integral, it will actually place the _floor_ of x / 2.0 in covers_half.

// I propose, therefore, that invoking ceil()/ceilf() on an integral value trigger a warning. Alternatively, that some more complicated conditions trigger the warning, e.g. invoking ceil() on the result of an integer division or integer multiplication.


