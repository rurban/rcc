/* GCC Bug #65673 - Compound literal with initializer for zero-sized array drops other initializers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65673
 */


struct s {
    int y;
    unsigned long *x;
};

struct s foo = {
    .x = (unsigned long [SZ]){},
};
//     .ident    "GCC: (Ubuntu 4.9.1-16ubuntu6) 4.9.1"
// If SZ is zero, the initializer for .y (".y = 25") member is dropped as well:
//     .ident    "GCC: (Ubuntu 4.9.1-16ubuntu6) 4.9.1"
// I'd add that this was a reduced test case from a bigger aggregate type - which was an array of such structures. When one of the elements became unused and the size of the bitmap (which was the purpose of the compound literal initializer) was set to zero, the whole array lost its initializers - i.e., even other 'struct s' members of the array, not just the member with a zero-sized array compound literal.


