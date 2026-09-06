/* GCC Bug #125524 - warning for __counted_by(len) field (or sanitizer) being changed not atomically
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125524
 */


#include <stdio.h>
struct buffer {
    int len;
    char * ptr __attribute((counted_by(len)));
};
int ltrim(struct buffer * const buf) {
    while (buf->len > 0 && *buf->ptr == ' ') {
        buf->len--;
        buf->ptr++;
    }
    return buf->len;
}
int main() {
    struct buffer buf = {.ptr = " 123", .len = 4};
    int ret = ltrim(&buf);
//     fprintf(stderr, "ret: %u\n", ret);
    return 0;
}
// <a href="https://godbolt.org/z/h7d7W3K45">https://godbolt.org/z/h7d7W3K45</a>
// The previous bug was closed on the basis of:
//   <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html#index-counted_005fby">https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html#index-counted_005fby</a>
// _In addition to the above requirements, there is one more requirement between this pair if and only if p->array is an array that is pointed by the pointer field:
// p->array and p->count can only be changed by changing the whole structure at the same time._
// However, the cited text is followed by:
// _It’s the programmer’s responsibility to make sure the above requirements to be kept all the time.  Otherwise the compiler reports warnings and the results of the array bound sanitizer and the __builtin_dynamic_object_size built-in are undefined._
// -> GCC is *not* reporting a warning
// -> CLANG DTRT with this code
// -> how can adding the attribute __counted_by__() make what, to the best of my knowledge is a well defined and common coding idiom, undefined?


