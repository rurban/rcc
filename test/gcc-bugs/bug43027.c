/* GCC Bug #43027 - #pragma rejected inside enum defn
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=43027
 */
/* { dg-do compile } */


enum {
    #pragma message "Hello from enum"
    VALUE1, VALUE2 };
// ==end code==
// Using built-in specs.
// Target: i686-pc-linux-gnu
// Configured with: ../configure --prefix=/usr/local/pkg/gcc-4.4.2 --with-gmp=/usr/local/pkg/gmp-4.2.4 --with-mpfr=/usr/local/pkg/mpfr-2.3.2 --disable-nls --disable-bootstrap --enable-languages=c,c++,fortran
// Thread model: posix
// I have tried a few other gcc versions (on both 32- and 64-bit targets) and any I can find that are new enough to support "#pragma message" produce similar results.  It is my suspicion that any supported (non-ignored) pragma is going to trigger the same error.

// While the utility of the #pragma inside the enum definition is questionable, I am not aware of anything that would prohibit its placement there (but am willing to be enlightened).  In fact, the struct and union definitions below appear to work just fine:
   struct S {
      #pragma message "Hello from struct"
      int i;
      float f;
    };
    union U {
      #pragma message "Hello from union"
      int i;
      float f;
    };


