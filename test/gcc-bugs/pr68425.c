/* GCC Bug #68425 - Enhanced error message when an array is initialized with too many elements
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68425
 */
/* { dg-do compile } */


const int array[2] = { 1, 2, 3 };
// When there are many elements, it is sometimes interesting to know whether we initialized the array with too many or not enough elements. I would expect such a message instead:
//     error: too many initializers for 'const int [2]' (3 elements, expected 2)
        const int array[2] = { 1, 2, 3 };
// I guess that it is rather cheap to implement and can help to diagnose the actual problem faster.


