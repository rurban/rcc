/* GCC Bug #98684 - -Wswitch interaction with "case X ... Y" -- warns for X and Y not being in the enum, but not X+1, X+2, ... Y-1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98684
 */
/* { dg-do compile } */


enum myenum {
};

int z(enum myenum e) {
    switch (e) {
            return 42;
            return 0;
            return 5;
    }
    return 0;
}
// GNU C17 (Compiler-Explorer-Build) version 11.0.0 20210106 (experimental) (x86_64-linux-gnu)
// (Side note: it would be useful to have a syntax in the enum definition to say "all values X ... Y are valid for this enum type", so as to be able to tell the compiler that without having to write them all out.)


