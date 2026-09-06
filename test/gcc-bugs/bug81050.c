/* GCC Bug #81050 - ICE with -fexec-charset=utf-16
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81050
 */
/* { dg-do compile } */


#include <stdio.h>
     int main()
     {
       char a = 'a';
//        printf("Size of char : %d\n",sizeof(a));
//        printf("Size of char : %d\n",sizeof('a'));
       return 0;
     }
       char a = 'a';
//        printf("Size of char : %d\n",sizeof('a'));
     }
 2. The trailing } on a line of its own.

// This is with GCC 7.1.1 on x86_64-suse-linux; similarly was reported with


