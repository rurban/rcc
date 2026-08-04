/* GCC Bug #67569 - wrong type in error message with float on x86 (387)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67569
 */


// /* Erroneous compiler error message */

 int main(void)
{
   int   *A = (int *)533 ;
   float  F = 1.F        ;
   A - F                 ;   
   return 0             ;
}

// /*
   Results:
//    In function 'main':
//    main_comp_err_msg.c:6:5: 
//    erreur: invalid operands to binary - (have 'int *' and 'long double')
//        A-F;
//          ^
   Note:
//    The variable F is not of type long double.        
//    Microsoft Windows XP Profesional version2002 Service Pack 3.
//    Gcc 4.8.0 win32 mingw32 -std=C99 
// */


