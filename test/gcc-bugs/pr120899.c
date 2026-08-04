/* GCC Bug #120899 - Duplicate diagnostics about jump into scope of pointer-to-VLA type and variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120899
 */


int main(int argc, char *argv[]) {
        goto lab; /* { dg-error "jump into scope of identifier with variably modified type" } */
        typedef int t[argc];
        t *a;
lab:    ;
}
// ```
// as a is a pointer to a VLA defined type.
// From .original:
    typedef int <anon>[0:(sizetype) ((long int) SAVE_EXPR <argc> + -1)];
  (void) SAVE_EXPR <argc>;
    int[0:(sizetype) ((long int) SAVE_EXPR <argc> + -1)] * a;
// So maybe we don't need to print an diagnostic out for the typedef with an anonymous name.


