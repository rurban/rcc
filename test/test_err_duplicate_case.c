/* GCC rejects duplicate case values ("duplicate case value"). rcc accepted
 * them, which broke Tcl's configure probe for TCL_WIDE_INT_IS_LONG:
 *   switch (0) { case 1: case (sizeof(long long) == sizeof(long)): ; }
 * is meant to FAIL to compile where long and long long are the same size.
 * rcc compiled it, configure decided Tcl_WideInt is `long long`, and
 * Tcl_GetInt("0") failed ((Tcl_WideInt)ULONG_MAX is -1), so Tcl could not
 * even source init.tcl. Here the duplicate is a constant expression that is
 * equal on every target.
 */
int main(void) {
    switch (0) {
    case 2:
    case 1 + 1:
        break;
    }
    return 0;
}
