/* GCC Bug #15767 - ppc-linux type attribute aligned, packed on vector types behaves wrongly
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=15767
 */


#define Align(N) __attribute__ ((aligned (N)))
#define Vector(N)  __attribute__ ((vector_size (N)))

typedef Align (2) Vector (16) unsigned char myChar;
typedef Vector (16) Align (2) unsigned char otherChar;

struct __attribute__((packed)) A {
   char       a;
   vector int b;
} sa;

struct B {
    char      c;
    myChar    a;
    char      b;
} sb;
struct C {
    char      c;
    otherChar a;
    char      b;
} sc;

int main (void)
{
    __builtin_printf ("sizeof   sa:    %2zu\n", sizeof (sa));
    __builtin_printf ("alignof  sa:    %2zu\n", __alignof__ (sa));
    __builtin_printf ("offsetof sa.b:  %2zu\n\n",
                      __builtin_offsetof (struct A, b));

    __builtin_printf ("sizeof   sb:    %2zu\n", sizeof (sb));
    __builtin_printf ("alignof  sb:    %2zu\n", __alignof__ (sb));
    __builtin_printf ("offsetof sb.a:  %2zu\n",
                       __builtin_offsetof (struct B, a));
    __builtin_printf ("offsetof sb.b:  %2zu\n\n",
                       __builtin_offsetof (struct B, b));

    __builtin_printf ("sizeof   sc:    %2zu\n", sizeof (sc));
    __builtin_printf ("alignof  sc:    %2zu\n", __alignof__ (sc));
    __builtin_printf ("offsetof sc.a:  %2zu\n",
                       __builtin_offsetof (struct C, a));
    __builtin_printf ("offsetof sc.b:  %2zu\n",
                       __builtin_offsetof (struct C, b));
}
// t.c: In function ‘main’:
// t.c:28:49: warning: ISO C does not allow ‘__alignof__ (expression)’ [-Wpedantic]
     __builtin_printf ("alignof  sa:    %2zu\n", __alignof__ (sa));
//                                                  ^~~~~~~~~~~
// t.c:33:49: warning: ISO C does not allow ‘__alignof__ (expression)’ [-Wpedantic]
     __builtin_printf ("alignof  sb:    %2zu\n", __alignof__ (sb));
//                                                  ^~~~~~~~~~~
// t.c:40:49: warning: ISO C does not allow ‘__alignof__ (expression)’ [-Wpedantic]
     __builtin_printf ("alignof  sc:    %2zu\n", __alignof__ (sc));
//                                                  ^~~~~~~~~~~
sizeof   sa:    17
// alignof  sa:     1
// offsetof sa.b:   1
sizeof   sb:    48
// alignof  sb:    16
// offsetof sb.a:  16
// offsetof sb.b:  32
sizeof   sc:    20
// alignof  sc:     2
// offsetof sc.a:   2
// offsetof sc.b:  18
// $


