/* GCC Bug #97687 - -Wfatal-errors prints some notes but not others
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97687
 */
/* { dg-do compile } */


#define FOO foo
    typedef int FOO;
    typedef short FOO;
//         3 | typedef short FOO;
//         2 | typedef int FOO;
//         3 | typedef short FOO;
// -Wfatal-errors is printing some notes but not others. It would be much more useful (to me) if it printed all of them, although in bugs #33952 and #37773 you claim that -Wfatal-errors is not for users, so I'm not really sure what you want it to do here.


