/* GCC Bug #50422 - -Wswitch warns about unhandled cases in nested switches
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50422
 */


typedef enum {
//         NAUGHT,
//         ONE,
//         TWO,
} foo_t;
// int
// test(foo_t arg)
{
        switch (arg) {
        case NAUGHT:
        case ONE:
                switch (arg) {
                case NAUGHT:
                        return 0;
                case ONE:
                        return 1;
                }
                break;
        case TWO:
                return 2;
        }
        return -1;
}
// EOF
// <stdin>: In function 'test':
// <stdin>:13:3: warning: enumeration value 'TWO' not handled in switch [-Wswitch]
// This is _similar_ to <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - spurious warnings about unhandled cases in switches (need VRP and control flow in front-end)"
//    href="show_bug.cgi?id=23577">bug 23577</a> but not quite as nested switches should be easier to handle than separate fragments.


