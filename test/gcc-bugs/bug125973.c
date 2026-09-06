/* GCC Bug #125973 - -- with static bool compound literal is mishandled
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125973
 */


#include<stdio.h>
int main(){
    for(unsigned u=0;u<5;++u){
        static bool b={};
        static int i={};
        printf("%i %i %i %i\n",--(static bool){},--b,--(static int){},--i);
    }
}
// The result is:
// 1 1 -1 -1
// 1 0 -2 -2
// 1 1 -3 -3
// 1 0 -4 -4
// 1 1 -5 -5
// It appears that (static bool){} gets reinitialized each loop iteration, but (static int){} does not. This difference is surely not intended, and not reinitializing appears to be the correct result:
// <span class="quote">> Otherwise, if the storage duration is automatic, the initializer is evaluated
// > at each evaluation of the compound literal; if the storage duration is static
// > or thread the initializer is (as if) evaluated once prior to program startup.</span >
// Section 6.5.3.6 "Compound literals" Paragraph 7 ISO/IEC 9899:2024
// Enumerated types compatible with bool appear to have the same issue, and putting the object inside of a structure type appears to remove the issue. Also, <a class="bz_bug_link 
//           bz_status_SUSPENDED "
//    title="SUSPENDED - Incorrect reinitialization of compound literal"
//    href="show_bug.cgi?id=18814">PR18814</a> looks to be invalid given this wording. The initializers of compound literals which designate objects with automatic storage duration get evaluated each time, just as with ordinary declarations and those cause reinitialization to happen.


