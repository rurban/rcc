/* GCC Bug #59220 - bogus warning: packed attribute is unnecessary on an overaligned char
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59220
 */


extern int printf (const char*, ...);

typedef char C __attribute__ ((aligned (4)));

struct S {
   C c __attribute__ ((packed));
};

int main (void) {
    struct S s;
    printf ("%zu %zu %zu\n",
             __alignof__ (s), __alignof__ (s.c), __alignof__ (C));
    return 0;
} 
// v.c:6:6: warning: packed attribute is unnecessary for 'c' [-Wattributes]
//    C c __attribute__ ((packed));
//       ^
// 1 1 4


