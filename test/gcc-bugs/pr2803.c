/* GCC Bug #2803 - casts in asm act as lvalues
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=2803
 */


// /* taken from longlong.h, GNU MP */

     #define add_ssaaaa(sh, sl, ah, al, bh, bl) \
       __asm__ ("addl %5,%1\n\tadcl %3,%0"                          \
//                 : "=r" ((USItype)(sh)), "=&r" ((USItype)(sl))       \
//                 : "%0" ((USItype)(ah)), "g" ((USItype)(bh)),        \
//                   "%1" ((USItype)(al)), "g" ((USItype)(bl)))

     typedef unsigned long   USItype;
     typedef unsigned long   UWtype;
     typedef unsigned short  UHWtype;

     #define W_TYPE_SIZE     32
//      UWtype
//      add (UWtype * sum, UWtype a, UWtype b, UWtype carry)

     {
             USItype new_carry;

//              add_ssaaaa(new_carry, *sum, 0, a, 0, b);
             if (carry == 0) {
                     return new_carry;
             }
//              add_ssaaaa(new_carry, *sum, new_carry, *sum, 0, carry);
             return new_carry;
     }
//      30% gcc -W -Wall -c 2803.c
//      31% g++ -W -Wall -c 2803.c
//      2803.c: In function `UWtype add(UWtype*, long unsigned int, long unsigned int,
        long unsigned int)':
//      2803.c:23: non-lvalue in assignment
//      2803.c:23: non-lvalue in assignment
//      2803.c:27: non-lvalue in assignment
//      2803.c:27: non-lvalue in assignment
//      32%
//  I don't know enough about the idiosyncracies of extended asm syntax to
//  say whether this is a bug.  This message is for clarification only.
//  Phil
//  -- 
//  pedwards at disaster dot jaj dot com  |  pme at sources dot redhat dot com
//  devphil at several other less interesting addresses in various dot domains
//  The gods do not protect fools.  Fools are protected by more capable fools.


