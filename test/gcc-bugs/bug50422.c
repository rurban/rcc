/* GCC Bug #50422 - -Wswitch warns about unhandled cases in nested switches
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50422
 */
/* { dg-do compile } */
/* { dg-options "-Wall" } */

typedef enum {
        NAUGHT,
        ONE,
        TWO,
} foo_t;

int
test(foo_t arg)
{
        switch (arg) {
        case NAUGHT:
        case ONE:
                switch (arg) { /* { dg-warning "not handled in switch" } */
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

// GCC warns about the inner switch not handling enumeration value 'TWO',
// even though control flow guarantees arg can only be NAUGHT or ONE at that
// point (the outer switch already dispatched on those two cases).  This is
// _similar_ to bug 23577 but not quite, as nested switches should be easier
// to handle than separate fragments.
