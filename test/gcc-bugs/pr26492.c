/* GCC Bug #26492 - -Wstrict-aliasing=2 warns about explicitly allowed cast to pointer to union.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=26492
 */


extern void printf(const char*, ...);

  #define noalias(type, ptr) \
    (((union { type __x__; __typeof__(*(ptr)) __y__;} *)(ptr))->__x__)

  typedef unsigned short usa[2];
//   int
//   main ()
  {
      int a = 0x12345678;
      printf ("%x\n", a);
//       noalias(usa, &a)[1] = 0;
      printf ("%x\n", a);

      return 0;
  }
// Here's what I get:
//   linux1.codesourcery.com% ./a.out
//   12345678
//   5678
// which is I think what you expected.  
// "
// This still works, but with gcc 4.0.3 and -Wstrict-aliasing=2 I see:
// (using #include <stdio.h> instead of extern void printf...)
// alias.c: In function 'main':
// 12345678
// 5678
// It would be nice if the warning would not trigger in this case. Although the documentation does not say it must trigger, this is claimed to be a documented extension and should work warning-free IMHO.


