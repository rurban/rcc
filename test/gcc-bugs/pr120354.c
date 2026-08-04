/* GCC Bug #120354 - Flexible array union in the middle is not reported by -Wflex-array-member-not-at-end option when flexible array member is not the last one
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120354
 */


union X {
    int x[];
    int y;
};

struct T {
    union X x;
    int plug;
};
// ```
// In this example, the flexible array member in `union X` is not the last element. However, it is still a flexible array union. When using this union to define a member in the middle of another struct, the usage is not reported.
// This is also true when referencing a flexible array struct in the union.
// ```
struct Q {
    int len;
    int data[];
};

union Y {
    struct Q q;
    int y;
};

struct S {
    union Y y;
    int plug;
};
// ```


