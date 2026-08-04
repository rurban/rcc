/* GCC Bug #16804 - Function pointer assignment/initialization (-Wc++-compat warning missing from -Wincompatible-pointer-types)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=16804
 */


enum Moo { Baa, Oink };
static unsigned int quack (void) { return 0; }
enum Moo (*Miau) (void) = quack;
// <span class="quote">> gcc-3.4 -Wall -c ~/foo.c
// > gcc-2.95.2 -Wall -c ~/foo.c
// > /opt/SUNWspro/bin/cc -V -c  ~/foo.c</span >
// cc: Sun WorkShop 6 update 2 C 5.3 Patch 111679-08 2002/05/09
// acomp: Sun WorkShop 6 update 2 C 5.3 Patch 111679-08 2002/05/09
// "/home/welinder/foo.c", line 3: warning: initialization type mismatch
// <span class="quote">> ./check ~/foo.c</span >
// /home/welinder/foo.c:3:27: warning: incorrect type in initializer (different
// base types)
// /home/welinder/foo.c:3:27:    expected enum Moo ( *[addressable] [toplevel] Miau
// )( ... )
// /home/welinder/foo.c:3:27:    got unsigned int ( static [addressable] [toplevel]
// *<noident> )( ... )
// Note the "unsigned".  Without that, I do get a warning.


