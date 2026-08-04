/* GCC Bug #448 - <stdint.h>-related issues (C99 issues)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=448
 */


// /* There is some amount of overlap with <sys/types.h> as known by inet code */
#ifndef __int8_t_defined
# define __int8_t_defined
typedef signed char             int8_t;
// ...
// whereas older glibc used in some places a convention of the form
# ifndef intptr_t
typedef long int               intptr_t;
// ...
// You should arrange for error messages mentioning definitions from the 
// pragma to give the line number of the #pragma in <stdint.h> as the line 
// number of both the typedefs and the macro definitions, to avoid making 
// this approach too confusing.


