/* GCC Bug #29970 - mixing ({...}) with VLA leads to massive breakage
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=29970
 */


#include <stdio.h>

#define each(item, array) \
(typeof(*(array)) *foreach_p = (array), *foreach_p2 = foreach_p, (item) = {}; \
foreach_p < &((foreach_p2)[sizeof(array)/sizeof(*array)]); \
++foreach_p)if((__builtin_memcpy(&(item), foreach_p, sizeof((item)))), 0){}else

#define range1(_stop) (({ \
 typeof(_stop) stop = _stop; \
 struct{typeof((stop)) array[stop];}p = {}; \
	if(stop < 0){ \
		for(size_t i = 0; i > stop; --i) \
   p.array[-i] = i; \
 }else{ \
		for(size_t i = 0; i < stop; ++i) \
   p.array[i] = i; \
 } \
// 	p; \
}).array)

int main(){
 char group[][4] = {
// 		"egg",
// 		"one",
// 		"two",
// 		"moo",
 };
	for each(x, group){
// 		puts(x);
 }
 return sizeof(range1(6));
}
// which I was able to minify to:
void f()
{
  (void)({
    int x = 1;
    struct {
      int array[x];
    } p;
//     p;
  });
}
// which roughly matches what testcase 2 does.


