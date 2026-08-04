/* GCC Bug #66918 - Disable "inline function declared but never defined" warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66918
 */


// /*first.h*/
inline void f(void);

// /*second.h*/
#include "first.h"
inline void f(void){}
// Unfortunately, if only the first header is included, gcc's generating this unsilencable warning unless I drop the `inline` from the prototype, but if I do and if I then also include the second header with the definition, then the prototype without the inline will turn into an unwanted instantiation and linker errors down the road.


