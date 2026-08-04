/* GCC Bug #102291 - wrong overflow warning for compound expression conversion and bit_and expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102291
 */


// /* #include <assert.h> */
extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
    __attribute__ ((__nothrow__ , __leaf__))
            __attribute__ ((__noreturn__));
#define assert(expr)                          \
  ((void) sizeof ((expr) ? 1 : 0), __extension__ ({         \
      if (expr)                             \
//         ; /* empty */                           \
      else                              \
        __assert_fail (#expr, __FILE__, __LINE__, __ASSERT_FUNCTION);   \
    }))
#define __ASSERT_FUNCTION    __extension__ __PRETTY_FUNCTION__
// /* end assert.h */

typedef unsigned long ulong;
typedef unsigned char uchar;

#define FIT8(c) assert(((sizeof(c) == 1) || (((ulong) (c)) >> 8) == 0))
#define BE8a(c) (FIT8(c), ((uchar) (c)))
#define BE8b(c) (         ((uchar) (c)))
#define NUM(c) ((c) | 0)

#define TESTER(old, new) ((((ulong)(old)) << 6) | (((uchar) NUM(new)) & 0x3f))

ulong testera(ulong ul) {
    return TESTER(ul, BE8a(0x80));
}

ulong testerb(ulong ul) {
    return TESTER(ul, BE8b(0x80));
}
// % gcc -c test.c 
// test.c: In function 'testera':
// test.c:24:49: warning: overflow in conversion from 'int' to 'long unsigned int' changes value from '((((void)4, (({...}))), 128)) & 63' to '0' [-Woverflow]
//    24 | #define TESTERa(old, new) ((((ulong)(old)) << 6) | (((uchar) NUM(new)) & 0x3f))
//       |                                                  ^
// test.c:27:12: note: in expansion of macro 'TESTERa'
//    27 |     return TESTERa(ul, BE8a(0x80));
//       |            ^~~~~~~
// %


